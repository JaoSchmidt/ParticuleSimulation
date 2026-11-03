/**
 * @file main-big-entity.cpp
 * @brief Mass-read benchmark: big entities, plain objects vs. archetypes.
 *
 * Unlike main.cpp nothing moves or collides here. Every entity carries 1 KiB of
 * data split over four components, and every frame two read-only passes walk
 * all of them:
 *
 *  - readHot: reads a single component (1/4 of each entity);
 *  - readAll: reads every component of each entity.
 *
 * Both flavours hold the same component structs and run the same summing loop.
 * The only difference is the memory layout:
 *
 *  - object: one std::vector of whole entities (array of structs);
 *  - archetype: one std::vector per component (reg::ArcheRegistry columns).
 *
 * The flavour is chosen at configure time with SIM_MODE. This target isn't
 * built when SIM_MODE is sparse_set. The sums are shown in the "Simulation"
 * panel and must match across flavours for the same entity count.
 *
 * The components are not in WorldComponents, so they can't live in the world
 * scene's registry. The archetype flavour owns a separate registry instead, and
 * both flavours run their passes from the script.
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
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

static_assert(Simulation::mode != SimulationMode::SparseSet,
              "The big entity simulation has no sparse set version");

// static_assert(pain::optimizationLevel == 2, "wrong opt");
namespace
{
// =============================================================== //
// Components
// =============================================================== //
namespace bigtag
{
struct Stats;
struct Inventory;
struct History;
struct Appearance;
} // namespace bigtag

/// Unsigned integers keep the sums exact, and let the compiler vectorize the
/// loops so that memory access dominates the measurement, not the arithmetic.
using Payload = std::array<std::uint32_t, 64>;

struct Stats {
  using tag = bigtag::Stats;
  Payload m_data;
};
struct Inventory {
  using tag = bigtag::Inventory;
  Payload m_data;
};
struct History {
  using tag = bigtag::History;
  Payload m_data;
};
struct Appearance {
  using tag = bigtag::Appearance;
  Payload m_data;
};

using BigEntityComponents =
    reg::CompileTimeBitMask<bigtag::Stats, bigtag::Inventory, bigtag::History,
                            bigtag::Appearance>;

/// Same value for the same entity, component and slot in both flavours.
template <typename Component>
Component makeComponent(std::size_t entityIndex, std::uint32_t componentIndex)
{
  Component component;
  for (std::uint32_t slot = 0; slot < component.m_data.size(); ++slot)
    component.m_data[slot] = static_cast<std::uint32_t>(entityIndex) * 31U +
                             componentIndex * 17U + slot;
  return component;
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

/// Whole entities side by side, so a read of one component steps over the rest.
class ObjectStorage
{
public:
  struct Object {
    Stats m_stats;
    Inventory m_inventory;
    History m_history;
    Appearance m_appearance;
  };
  static_assert(sizeof(Object) == 4 * sizeof(Payload));

  void rebuild(int count)
  {
    // Free the old entities first, so two generations are never held at once
    m_objects = {};
    m_objects.reserve(static_cast<std::size_t>(count));
    for (std::size_t i = 0; i < static_cast<std::size_t>(count); ++i)
      m_objects.push_back(
          {makeComponent<Stats>(i, 0), makeComponent<Inventory>(i, 1),
           makeComponent<History>(i, 2), makeComponent<Appearance>(i, 3)});
  }

  std::uint64_t readHot() const
  {
    PROFILE_SCOPE("BigEntity::readHot");
    std::uint64_t sum = 0;
    for (const Object &object : m_objects)
      sum += sumPayload(object.m_stats.m_data);
    return sum;
  }

  std::uint64_t readAll() const
  {
    PROFILE_SCOPE("BigEntity::readAll");
    std::uint64_t sum = 0;
    for (const Object &object : m_objects) {
      sum += sumPayload(object.m_stats.m_data);
      sum += sumPayload(object.m_inventory.m_data);
      sum += sumPayload(object.m_history.m_data);
      sum += sumPayload(object.m_appearance.m_data);
    }
    return sum;
  }

private:
  std::vector<Object> m_objects;
};

/// One array per component, so a read of one component touches only that array.
class ArchetypeStorage
{
public:
  void rebuild(int count)
  {
    // emplace() destroys the old registry first, so two generations are never
    // held at once
    Registry &registry = m_registry.emplace();
    for (std::size_t i = 0; i < static_cast<std::size_t>(count); ++i)
      registry.createComponents(
          registry.createEntity(), makeComponent<Stats>(i, 0),
          makeComponent<Inventory>(i, 1), makeComponent<History>(i, 2),
          makeComponent<Appearance>(i, 3));
  }

  std::uint64_t readHot() const
  {
    PROFILE_SCOPE("BigEntity::readHot");
    std::uint64_t sum = 0;
    for (const auto &chunk : m_registry->queryConst<Stats>()) {
      const Stats *stats = std::get<0>(chunk.arrays);
      for (std::size_t i = 0; i < chunk.count; ++i)
        sum += sumPayload(stats[i].m_data);
    }
    return sum;
  }

  std::uint64_t readAll() const
  {
    PROFILE_SCOPE("BigEntity::readAll");
    std::uint64_t sum = 0;
    for (const auto &chunk :
         m_registry->queryConst<Stats, Inventory, History, Appearance>()) {
      const auto [stats, inventory, history, appearance] = chunk.arrays;
      for (std::size_t i = 0; i < chunk.count; ++i) {
        sum += sumPayload(stats[i].m_data);
        sum += sumPayload(inventory[i].m_data);
        sum += sumPayload(history[i].m_data);
        sum += sumPayload(appearance[i].m_data);
      }
    }
    return sum;
  }

private:
  using Registry = reg::ArcheRegistry<BigEntityComponents>;
  std::optional<Registry> m_registry;
};

constexpr bool isObjectMode = Simulation::mode == SimulationMode::Object;
using Storage =
    std::conditional_t<isObjectMode, ObjectStorage, ArchetypeStorage>;
constexpr const char *storageName =
    isObjectMode ? "BigEntitySimulationObj" : "BigEntitySimulationArchetype";

// =============================================================== //
// Root script
// =============================================================== //
class BigEntitySimulation : public pain::WorldObject
{
public:
  BigEntitySimulation(reg::Entity entity, pain::Scene &scene, int entityCount)
      : pain::WorldObject(entity, scene)
  {
    setEntityCount(entityCount);
  }

  void setEntityCount(int count)
  {
    m_entityCount = std::clamp(count, 0, Simulation::bigEntityMaxCount);
    m_storage.rebuild(m_entityCount);
  }

  void renderControls()
  {
    ImGui::Text("Storage: %s", storageName);
    ImGui::Text("Entity size: %zu bytes", 4 * sizeof(Payload));
    ImGui::Text("Hot read sum: %llu",
                static_cast<unsigned long long>(m_hotSum));
    ImGui::Text("Full read sum: %llu",
                static_cast<unsigned long long>(m_fullSum));
    int requestedCount = m_entityCount;
    if (ImGui::InputInt("Amount", &requestedCount, 1024, 16 * 1024,
                        ImGuiInputTextFlags_EnterReturnsTrue))
      setEntityCount(requestedCount);
  }

  void onUpdate(pain::DeltaTime dt)
  {
    UNUSED(dt)
#ifdef PROFILING
    updateProfilingRounds();
#endif
    // Kept in members and shown in the panel, so the reads can't be optimized
    // away
    m_hotSum = m_storage.readHot();
    m_fullSum = m_storage.readAll();
  }

private:
#ifdef PROFILING
  /// Same rounds as main.cpp, except they stop once the largest count is done.
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
                                                         m_entityCount));
        m_profileFramesUntilClose = SimulationProfiling::profileFrameCount;
        PLOG_T("Starting profile on file {} with {} frame(s)", fileName,
               m_profileFramesUntilClose);
        PROFILE_OPEN(storageName, fileName.c_str());
      }
    } else if (--m_profileFramesUntilClose == 0) {
      PROFILE_CLOSE();
      if (m_entityCount >= Simulation::bigEntityMaxCount) {
        LOG_I("Profiling finished");
        m_profileFinished = true;
        exit(0);
      }
      setEntityCount(m_entityCount * 2);
      m_profileFramesUntilStart = SimulationProfiling::profileStartDelayFrames;
    }
  }

  /// 0: not waiting, >0: frames until profiling opens.
  int m_profileFramesUntilStart = 0;
  /// 0: not profiling, >0: frames until profiling closes.
  int m_profileFramesUntilClose = 0;
  bool m_profileFinished = false;
#endif

  Storage m_storage;
  int m_entityCount = 0;
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

  BigEntitySimulation &simulation =
      pain::Scene::emplaceScript<BigEntitySimulation>(
          scene.getEntity(), scene, Simulation::bigEntityStartCount);

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
