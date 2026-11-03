/**
 * @file main-mem-size-test.cpp
 * @brief Bytes stored vs. bytes accessed: plain objects vs. archetypes.
 *
 * The entity count stays fixed and the size of each entity changes instead:
 * every profiling round adds one more 256 byte struct, from 1 up to 12. Every
 * frame two read-only passes walk all the entities:
 *
 *  - readHot: reads a single struct, so the bytes accessed stay the same while
 *    the bytes stored grow;
 *  - readAll: reads every struct, so the bytes accessed grow with the bytes
 *    stored.
 *
 * Both flavours hold the same structs and run the same summing loop. The only
 * difference is the memory layout:
 *
 *  - object: one std::vector of whole entities (array of structs);
 *  - archetype: one std::vector per struct (reg::ArcheRegistry columns).
 *
 * The flavour is chosen at configure time with SIM_MODE. This target isn't
 * built when SIM_MODE is sparse_set. The sums are shown in the "Simulation"
 * panel and must match across flavours for the same struct count.
 *
 * Profile files are named <Storage>-<structs>.O<level>.json. Unlike the other
 * simulations, that number is the struct count and not the entity count.
 */

#include <pain.h>
#include <painless.h>

#include "CustomPanel.h"
#include "SimulationConfig.h"
#include "SimulationProfiling.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

static_assert(Simulation::mode != SimulationMode::SparseSet,
              "The memory size test has no sparse set version");

namespace
{
// =============================================================== //
// Components
// =============================================================== //
namespace memtag
{
template <std::size_t Index> struct Slot;
} // namespace memtag

/// Unsigned integers keep the sums exact, and let the compiler vectorize the
/// loops so that memory access dominates the measurement, not the arithmetic.
using Payload = std::array<std::uint32_t, 64>;

/// One arbitrary struct. Slot<0> is the one readHot reads.
template <std::size_t Index> struct Slot {
  using tag = memtag::Slot<Index>;
  Payload m_data;
};

template <std::size_t... Indices>
reg::CompileTimeBitMask<memtag::Slot<Indices>...>
    makeBitMask(std::index_sequence<Indices...>);

using MemSizeComponents = decltype(makeBitMask(
    std::make_index_sequence<Simulation::memSizeMaxStructCount>{}));

/// Same value for the same entity, struct and position in both flavours.
template <std::size_t Index> Slot<Index> makeSlot(std::size_t entityIndex)
{
  Slot<Index> slot;
  for (std::uint32_t i = 0; i < slot.m_data.size(); ++i)
    slot.m_data[i] = static_cast<std::uint32_t>(entityIndex) * 31U +
                     static_cast<std::uint32_t>(Index) * 17U + i;
  return slot;
}

inline std::uint64_t sumPayload(const Payload &payload)
{
  std::uint64_t sum = 0;
  for (std::uint32_t value : payload)
    sum += value;
  return sum;
}

// =============================================================== //
// Storage flavours
// =============================================================== //

/// The script picks the struct count at runtime, so each round reaches its
/// storage through this interface. It's called once per pass, never per entity.
class IStorage
{
public:
  virtual ~IStorage() = default;
  virtual std::uint64_t readHot() const = 0;
  virtual std::uint64_t readAll() const = 0;
};

/// Whole entities side by side, so a read of one struct steps over the rest.
template <typename Indices> class ObjectStorage;
template <std::size_t... Indices>
class ObjectStorage<std::index_sequence<Indices...>> : public IStorage
{
public:
  struct Object : Slot<Indices>... {
  };
  static_assert(sizeof(Object) == sizeof...(Indices) * sizeof(Payload));

  explicit ObjectStorage(std::size_t count)
  {
    m_objects.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
      m_objects.push_back(Object{makeSlot<Indices>(i)...});
  }

  std::uint64_t readHot() const override
  {
    PROFILE_SCOPE("MemSize::readHot");
    std::uint64_t sum = 0;
    for (const Object &object : m_objects)
      sum += sumPayload(static_cast<const Slot<0> &>(object).m_data);
    return sum;
  }

  std::uint64_t readAll() const override
  {
    PROFILE_SCOPE("MemSize::readAll");
    std::uint64_t sum = 0;
    for (const Object &object : m_objects)
      sum += (sumPayload(static_cast<const Slot<Indices> &>(object).m_data) +
              ...);
    return sum;
  }

private:
  std::vector<Object> m_objects;
};

/// One array per struct, so a read of one struct touches only that array.
template <typename Indices> class ArchetypeStorage;
template <std::size_t... Indices>
class ArchetypeStorage<std::index_sequence<Indices...>> : public IStorage
{
public:
  explicit ArchetypeStorage(std::size_t count)
  {
    for (std::size_t i = 0; i < count; ++i)
      m_registry.createComponents(m_registry.createEntity(),
                                  makeSlot<Indices>(i)...);
  }

  std::uint64_t readHot() const override
  {
    PROFILE_SCOPE("MemSize::readHot");
    std::uint64_t sum = 0;
    for (const auto &chunk : m_registry.queryConst<Slot<0>>()) {
      const Slot<0> *slots = std::get<0>(chunk.arrays);
      for (std::size_t i = 0; i < chunk.count; ++i)
        sum += sumPayload(slots[i].m_data);
    }
    return sum;
  }

  std::uint64_t readAll() const override
  {
    PROFILE_SCOPE("MemSize::readAll");
    std::uint64_t sum = 0;
    for (const auto &chunk : m_registry.queryConst<Slot<Indices>...>()) {
      const std::tuple<const Slot<Indices> *...> slots = chunk.arrays;
      for (std::size_t i = 0; i < chunk.count; ++i)
        sum += (sumPayload(std::get<const Slot<Indices> *>(slots)[i].m_data) +
                ...);
    }
    return sum;
  }

private:
  reg::ArcheRegistry<MemSizeComponents> m_registry;
};

constexpr bool isObjectMode = Simulation::mode == SimulationMode::Object;
constexpr const char *storageName =
    isObjectMode ? "MemSizeSimulationObj" : "MemSizeSimulationArchetype";

/// Builds the storage whose entities hold StructCount structs.
template <std::size_t StructCount>
std::unique_ptr<IStorage> makeStorageOfSize(std::size_t entityCount)
{
  using Indices = std::make_index_sequence<StructCount>;
  using Storage = std::conditional_t<isObjectMode, ObjectStorage<Indices>,
                                     ArchetypeStorage<Indices>>;
  return std::make_unique<Storage>(entityCount);
}

template <std::size_t... Sizes>
std::unique_ptr<IStorage> makeStorage(std::size_t structCount,
                                      std::size_t entityCount,
                                      std::index_sequence<Sizes...>)
{
  std::unique_ptr<IStorage> storage;
  ((structCount == Sizes + 1
        ? void(storage = makeStorageOfSize<Sizes + 1>(entityCount))
        : void()),
   ...);
  return storage;
}

// =============================================================== //
// Root script
// =============================================================== //
class MemSizeSimulation : public pain::WorldObject
{
public:
  MemSizeSimulation(reg::Entity entity, pain::Scene &scene)
      : pain::WorldObject(entity, scene)
  {
    setStructCount(1);
  }

  void setStructCount(int count)
  {
    m_structCount = std::clamp(count, 1, Simulation::memSizeMaxStructCount);
    // Free the old entities first, so two generations are never held at once
    m_storage.reset();
    m_storage = makeStorage(
        static_cast<std::size_t>(m_structCount),
        static_cast<std::size_t>(Simulation::memSizeEntityCount),
        std::make_index_sequence<Simulation::memSizeMaxStructCount>{});
  }

  void renderControls()
  {
    ImGui::Text("Storage: %s", storageName);
    ImGui::Text("Entities: %d", Simulation::memSizeEntityCount);
    ImGui::Text("Entity size: %zu bytes",
                static_cast<std::size_t>(m_structCount) * sizeof(Payload));
    ImGui::Text("Hot read sum: %llu",
                static_cast<unsigned long long>(m_hotSum));
    ImGui::Text("Full read sum: %llu",
                static_cast<unsigned long long>(m_fullSum));
    int requestedCount = m_structCount;
    if (ImGui::InputInt("Structs", &requestedCount, 1, 1,
                        ImGuiInputTextFlags_EnterReturnsTrue))
      setStructCount(requestedCount);
  }

  void onUpdate(pain::DeltaTime dt)
  {
    UNUSED(dt)
#ifdef PROFILING
    updateProfilingRounds();
#endif
    // Kept in members and shown in the panel, so the reads can't be optimized
    // away
    m_hotSum = m_storage->readHot();
    m_fullSum = m_storage->readAll();
  }

private:
#ifdef PROFILING
  /// Same rounds as main-big-entity.cpp, except each one adds a struct instead
  /// of doubling the entities.
  void updateProfilingRounds()
  {
    if (m_profileFinished)
      return;
    if (m_profileFramesUntilStart == 0 && m_profileFramesUntilClose == 0)
      m_profileFramesUntilStart = SimulationProfiling::profileStartDelayFrames;

    if (m_profileFramesUntilStart > 0) {
      if (--m_profileFramesUntilStart == 0) {
        std::string fileName = SimulationProfiling::getNextAvailableFileName(
            SimulationProfiling::generateProfileFilename(storageName,
                                                         m_structCount));
        m_profileFramesUntilClose = SimulationProfiling::profileFrameCount;
        PLOG_T("Starting profile on file {} with {} frame(s)", fileName,
               m_profileFramesUntilClose);
        PROFILE_OPEN(storageName, fileName.c_str());
      }
    } else if (--m_profileFramesUntilClose == 0) {
      PROFILE_CLOSE();
      LOG_I("Sums with {} struct(s): hot = {}, full = {}", m_structCount,
            m_hotSum, m_fullSum);
      if (m_structCount >= Simulation::memSizeMaxStructCount) {
        LOG_I("Profiling finished");
        m_profileFinished = true;
        exit(0);
      }
      setStructCount(m_structCount + 1);
      m_profileFramesUntilStart = SimulationProfiling::profileStartDelayFrames;
    }
  }

  /// 0: not waiting, >0: frames until profiling opens.
  int m_profileFramesUntilStart = 0;
  /// 0: not profiling, >0: frames until profiling closes.
  int m_profileFramesUntilClose = 0;
  bool m_profileFinished = false;
#endif

  std::unique_ptr<IStorage> m_storage;
  int m_structCount = 1;
  std::uint64_t m_hotSum = 0;
  std::uint64_t m_fullSum = 0;
};
} // namespace

pain::Application *pain::createApplication()
{
  // Retrieve the context the player will alter when using the launcher
  IniConfig ini;
  ini.readAndUpdate();

  // Retrieve the app context defined inside "resources/InternalConfig.ini"
  InternalConfig internalIni;
  internalIni.readAndUpdate(ini.assetsPath.value);
  // Create the application + OpenGL + Event contexts
  Application *app = Application::createApplication( //
      {
          .title = internalIni.title.get().c_str(),     //
          .defaultWidth = ini.defaultWidth.get(),       //
          .defaultHeight = ini.defaultHeight.get(),     //
          .defaultZoom2d = internalIni.zoomLevel.get(), //
          .fullWindow = ini.fullwindow.get(),           //
          .fullScreen = ini.fullscreen.get(),           //
          .gridCellSize = internalIni.gridSize.get(),
      },
      {.swapChainTarget = internalIni.swapChainTarget.get()} //
  );

  pain::Scene &scene = app->getWorldScene();
  scene.createComponents(scene.getEntity(), cmp::Script{});
  scene.addSystem<pain::Systems::NativeScript<WorldComponents>>();

  // Nothing is drawn, but the render pipeline warns when there is no camera
  const pain::AppInit &cfg = app->getCurrentConfig();
  reg::Entity camera = scene.createEntity("Static2DCamera");
  scene.createComponents(camera, pain::Transform2dComponent{glm::vec2{0.F}},
                         pain::OrthoCameraComponent::create(
                             true, cfg.defaultWidth, cfg.defaultHeight,
                             cfg.defaultZoom2d, camera));

  MemSizeSimulation &simulation =
      pain::Scene::emplaceScript<MemSizeSimulation>(scene.getEntity(), scene);

  painless::customPanel::registerPanel("Simulation", 1.F,
                                       painless::InterfaceMenu::SIDEBAR);
  painless::customPanel::addToPanel(
      "Simulation", [&simulation]() { simulation.renderControls(); });

  PLOG_T("Optimization level = {}", optimizationLevel);
  if (Simulation::mode == SimulationMode::Archetype) {
    PLOG_T("Simulation mode = Archetype");
  } else if (Simulation::mode == SimulationMode::Object) {
    PLOG_T("Simulation mode = Object");
  }
  UIScene &uiScene = app->createUIScene();
  painless::Editor::create(uiScene, *app);
  return app;
}

#ifdef PLATFORM_IS_LINUX
int main(int argc, char *argv[])
#elif defined PLATFORM_IS_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine,
                   int nCmdShow)
#endif
{
  bool isSettingsGuiNeeded = pain::Pain::initiateIni();
  EndGameFlags flags;
  flags.restartGame = !isSettingsGuiNeeded;
  if (isSettingsGuiNeeded) {
    pain::Application *app = painless::createLauncher();
    flags = pain::Pain::runAndDeleteApplication(app);
  }
  while (flags.restartGame) {
    pain::Application *app = pain::createApplication();
    flags = pain::Pain::runAndDeleteApplication(app);
  }
  return 0;
}
