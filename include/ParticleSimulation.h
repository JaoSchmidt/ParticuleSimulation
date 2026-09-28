#pragma once
#include "Asteroid.h"
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

/** @brief Which implementation of the simulation is currently running. */
enum class SimulationMode {
  Object = 0, ///< Plain C++ objects, stepped and drawn by the script itself.
  Ecs = 1     ///< Entities and components, stepped by the registered systems.
};

/** @brief One wall, in the mode independent form. */
struct WallSpec {
  glm::vec2 m_position;
  glm::vec2 m_size;
};

/**
 * @brief Everything a simulation needs that does not depend on the mode.
 *
 * Built once by SimulationModeSwitch and handed to whichever flavour is being
 * created, so switching back and forth never rebuilds the camera, the mouse
 * pointer, the textures or the materials. All the asset managers key on a
 * name and hand back the existing entry, so sharing these is what keeps
 * repeated switching from piling up duplicates.
 */
struct SimulationAssets {
  reg::Entity m_camera = reg::Entity{0};
  reg::Entity m_mousePointer = reg::Entity{0};
  pain::Material *m_wallMaterial = nullptr;
  std::vector<pain::Material *> m_asteroidMaterials;
  std::vector<WallSpec> m_walls;
};

/**
 * @brief The slice of a simulation that SimulationModeSwitch drives.
 *
 * We only ever holds one of these
 */
class ParticleSimulationBase : public pain::WorldObject
{

public:
  ParticleSimulationBase(reg::Entity entity, pain::Scene &scene)
      : pain::WorldObject(entity, scene) {};
  virtual ~ParticleSimulationBase() = default;

  /// @brief Respawns every asteroid from the shared seed, keeping the count.
  virtual void resetSimulation() = 0;
  /// @brief Resizes the simulation to @p count asteroids and respawns them.
  virtual void setAsteroidCount(int count) = 0;
  /// @brief How many asteroids this flavour is currently simulating.
  ///
  /// The switcher carries the count across a swap, so the population does not
  /// jump back to the default every time the mode changes.
  virtual int asteroidCount() const = 0;
  /// @brief Draws the controls that drive this flavour.
  virtual void renderControls() = 0;

  /// @brief Releases what this flavour left in the world registry, if anything.
  ///
  /// Called by SimulationModeSwitch while this object is still alive, right
  /// before the replacement simulation is bound over it. The object flavour
  /// keeps everything in plain members and has nothing to give back; the ECS
  /// flavour owns entities and has to remove them.
  virtual void releaseWorldResources() {};
};

class ParticleSimulationObj : public ParticleSimulationBase
{

public:
  static ParticleSimulationObj &
  createScriptScene(pain::Scene &scene, const SimulationAssets &assets,
                    int startAsteroidCount);
  ParticleSimulationObj(reg::Entity entity, pain::Scene &scene,
                        reg::Entity orthocamera,
                        std::vector<Asteroid::Object> &&asteroid,
                        std::vector<Wall::Object> &&walls, reg::Entity mp,
                        std::vector<pain::Material *> &&asteroidMaterials);

  void onUpdate(pain::DeltaTime dt);
  void onRender(pain::RenderApi &rs, pain::DeltaTime currentTime);

  void setAsteroidCount(int count) override;
  void resetSimulation() override;
  int asteroidCount() const override { return m_asteroidCount; }
  void renderControls() override;

  std::vector<std::vector<int>> m_backgroundMap;
  std::shared_ptr<pain::Shader> m_texture_shader;
  reg::Entity m_mousePointer;
  reg::Entity m_orthoCamera;
  std::vector<Asteroid::Object> m_asteroids;
  std::vector<Wall::Object> m_walls;
  /// @brief Kept so resetSimulation() can respawn without the material
  ///        manager's lookups happening again.
  std::vector<pain::Material *> m_asteroidMaterials;
  int m_asteroidCount = Simulation::asteroidAmount;
  // Player m_player;
  const static unsigned starAmout = 0;
  const static unsigned asteroidAmount = Simulation::asteroidAmount;
};

class ParticleSimulationECS : public ParticleSimulationBase
{

public:
  static ParticleSimulationECS &
  createScriptScene(pain::Scene &scene, const SimulationAssets &assets,
                    int startAsteroidCount);
  ParticleSimulationECS(reg::Entity entity, pain::Scene &scene,
                        reg::Entity orthocamera,
                        std::vector<reg::Entity> &&asteroid,
                        std::vector<reg::Entity> &&walls, reg::Entity mp,
                        std::vector<pain::Material *> &&asteroidMaterials);

  void setAsteroidCount(int count) override;
  void resetSimulation() override;
  int asteroidCount() const override { return m_asteroidCount; }
  void renderControls() override;
  /// @brief Removes the wall and asteroid entities this flavour created.
  void releaseWorldResources() override;

  std::vector<std::vector<int>> m_backgroundMap;
  std::shared_ptr<pain::Shader> m_texture_shader;
  reg::Entity m_mousePointer;
  reg::Entity m_orthoCamera;
  std::vector<reg::Entity> m_asteroids;
  std::vector<reg::Entity> m_walls;
  /// @brief Kept so resetSimulation() can create the entities a larger amount
  ///        asks for without rebuilding the materials.
  std::vector<pain::Material *> m_asteroidMaterials;
  int m_asteroidCount = Simulation::asteroidAmount;
  // Player m_player;
  const static unsigned starAmout = 0;
  const static unsigned asteroidAmount = Simulation::asteroidAmount;
};
