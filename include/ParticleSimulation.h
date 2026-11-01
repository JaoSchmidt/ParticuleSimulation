#pragma once
#include "Asteroid.h"
#include "SparseSetAlternatives/GameObject.h"
#include "Wall.h"
#include <pain.h>
#include <painless.h>

/**
 * @brief Shared knobs for both simulation flavours.
 *
 * asteroidSeed, asteroidRadius and asteroidAmount are what the two modes have
 * in common: a single seed drives the same Gaussian draw sequence in both, so
 * asteroid i spawns at the same spot with the same velocity no matter which
 * implementation is running, and no matter how many asteroids are asked for.
 */
namespace Simulation
{
const static pain::RNG::SeedType asteroidSeed = 1;
const static float asteroidRadius = 0.1f;
const static unsigned asteroidAmount = 200;
/** Upper bound of the amount slider. Collision is O(n^2) here, so this is
 *  deliberately modest. */
const static int maxAsteroidCount = 2000;
} // namespace Simulation

/** @brief Which implementation of the simulation is running. */
enum class SimulationMode {
  Object = 0, ///< Plain C++ objects, stepped and drawn by the script itself.
  Ecs = 1     ///< Entities and components, stepped by the registered systems.
};

namespace Simulation
{
/** @brief The implementation built into this executable.
 *
 * Selected at compile time: change this value and rebuild to run the other
 * simulation. There is no runtime switch.
 */
constexpr SimulationMode mode = SimulationMode::Ecs;
} // namespace Simulation

/** @brief One wall, eq to cmp::Pos2d */
struct WallSpec {
  glm::vec2 m_position;
  glm::vec2 m_size;
};

/**
 * @brief Everything a simulation needs that does not depend on the mode.
 */
struct SimulationAssets {
  reg::Entity m_camera = reg::Entity{0};
  reg::Entity m_mousePointer = reg::Entity{0};
  pain::Material *m_wallMaterial = nullptr;
  std::vector<pain::Material *> m_asteroidMaterials;
  std::vector<WallSpec> m_walls;
};

/**
 * @brief The slice of a simulation that is bound to the scene's root entity.
 *
 * Only one of these is ever alive; which one is decided at compile time.
 */
class ParticleSimulationBase
{

public:
  ParticleSimulationBase(reg::Entity orthocamera,                           //
                         std::vector<pain::Material *> &&asteroidMaterials) //
      : m_orthoCamera(orthocamera),                                         //
        m_asteroidMaterials(std::move(asteroidMaterials)) {};
  virtual ~ParticleSimulationBase() = default;

  /// @brief Respawns every asteroid from the shared seed, keeping the count.
  virtual void resetSimulation() = 0;
  /// @brief Resizes the simulation to @p count asteroids and respawns them.
  virtual void setAsteroidCount(int count) = 0;
  /// @brief How many asteroids this flavour is currently simulating.
  virtual int asteroidCount() const = 0;
  /// @brief Draws the controls that drive this flavour.
  virtual void renderControls() = 0;

  std::vector<std::vector<int>> m_backgroundMap;
  std::shared_ptr<pain::Shader> m_texture_shader;
  reg::Entity m_orthoCamera;
  /// @brief Kept so resetSimulation() can respawn without the material
  ///        manager's lookups happening again.
  std::vector<pain::Material *> m_asteroidMaterials;
  int m_asteroidCount = Simulation::asteroidAmount;
  // Player m_player;
  const static unsigned starAmout = 0;
  const static unsigned asteroidAmount = Simulation::asteroidAmount;
};

class ParticleSimulationObj : public pain::WorldObject,
                              public ParticleSimulationBase

{

public:
  static ParticleSimulationObj &
  createScriptScene(pain::Scene &scene, const SimulationAssets &assets,
                    int startAsteroidCount);
  ParticleSimulationObj(reg::Entity entity, pain::Scene &scene,
                        reg::Entity orthocamera,
                        std::vector<Asteroid::Object> &&asteroid,
                        std::vector<Wall::Object> &&walls,
                        std::vector<pain::Material *> &&asteroidMaterials);

  void onEvent(const SDL_Event &e);
  void onUpdate(pain::DeltaTime dt);
  void onRender(pain::RenderApi &rs, pain::DeltaTime currentTime);

  void setAsteroidCount(int count) override;
  void resetSimulation() override;
  int asteroidCount() const override { return m_asteroidCount; }
  void renderControls() override;
  std::vector<Asteroid::Object> m_asteroids;
  std::vector<Wall::Object> m_walls;
  int m_profileFramesUntilClose = 0;
};

class ParticleSimulationArchetype : public pain::WorldObject,
                                    public ParticleSimulationBase

{
public:
  static ParticleSimulationArchetype &
  createScriptScene(pain::Scene &scene, const SimulationAssets &assets,
                    int startAsteroidCount);
  ParticleSimulationArchetype(
      reg::Entity entity, pain::Scene &scene, reg::Entity orthocamera,
      std::vector<reg::Entity> &&asteroid, std::vector<reg::Entity> &&walls,
      std::vector<pain::Material *> &&asteroidMaterials);

  void setAsteroidCount(int count) override;
  void resetSimulation() override;
  int asteroidCount() const override { return m_asteroidCount; }
  void renderControls() override;
  void onEvent(const SDL_Event &e);
  void onUpdate(pain::DeltaTime dt);
  std::vector<reg::Entity> m_asteroids;
  std::vector<reg::Entity> m_walls;
  int m_profileFramesUntilClose = 0;
};

class ParticleSimulationSparseSet : public SparseSetObject,
                                    public ParticleSimulationBase
{
public:
  static ParticleSimulationSparseSet &
  createScriptScene(tcc::SparseSetScene &scene, const SimulationAssets &assets,
                    int startAsteroidCount);
  ParticleSimulationSparseSet(
      reg::Entity entity, tcc::SparseSetScene &scene, reg::Entity orthocamera,
      std::vector<reg::Entity> &&asteroid, std::vector<reg::Entity> &&walls,
      std::vector<pain::Material *> &&asteroidMaterials);

  void setAsteroidCount(int count) override;
  void resetSimulation() override;
  int asteroidCount() const override { return m_asteroidCount; }
  void renderControls() override;
  void onEvent(const SDL_Event &e);
  void onUpdate(pain::DeltaTime dt);
  std::vector<reg::Entity> m_asteroids;
  std::vector<reg::Entity> m_walls;
  int m_profileFramesUntilClose = 0;
};
