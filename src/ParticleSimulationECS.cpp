#include "Assets/ManagerTexture.h"
#include "Asteroid.h"
#include "Misc/Basic2dPlayer.h"
#include "MousePointer.h"
#include "ParticleSimulation.h"
#include "Wall.h"
#include "imgui.h"
#include <algorithm>
#include <pain.h>
#include <painless.h>

ParticleSimulationECS &ParticleSimulationECS::createScriptScene(
    pain::Scene &scene, const SimulationAssets &assets, int startAsteroidCount)
{
  // Only the walls are built here, as entities. The camera, the mouse pointer,
  // the textures and the materials are shared with the other flavour and
  // arrive through @p assets, and the asteroids come from resetSimulation().
  // That split is what lets SimulationModeSwitch swap flavours without
  // rebuilding (or duplicating) anything that is not mode specific.
  std::vector<reg::Entity> walls;
  walls.reserve(assets.m_walls.size());
  for (const WallSpec &wall : assets.m_walls)
    walls.emplace_back(Wall::createECS(scene, wall.m_position, wall.m_size,
                                       *assets.m_wallMaterial));

  std::vector<reg::Entity> asteroids;

  ParticleSimulationECS &script =
      pain::Scene::emplaceScript<ParticleSimulationECS>(
          scene.getEntity(), scene, assets.m_camera, std::move(asteroids),
          std::move(walls), assets.m_mousePointer,
          // The assets are const, so the vector has to be copied rather than
          // moved. It holds pointers into the material manager, so this is
          // cheap and the copy is what keeps the shared assets reusable.
          std::vector<pain::Material *>(assets.m_asteroidMaterials));
  script.setAsteroidCount(startAsteroidCount);
  return script;
}

ParticleSimulationECS::ParticleSimulationECS(
    reg::Entity entity, pain::Scene &scene, reg::Entity orthocamera,
    std::vector<reg::Entity> &&asteroid, std::vector<reg::Entity> &&walls,
    reg::Entity mp, std::vector<pain::Material *> &&asteroidMaterials)
    : ParticleSimulationBase(entity, scene), m_mousePointer(std::move(mp)),
      m_orthoCamera(orthocamera), m_asteroids(std::move(asteroid)),
      m_walls(std::move(walls)),
      m_asteroidMaterials(std::move(asteroidMaterials)) {};

void ParticleSimulationECS::releaseWorldResources()
{
  // The walls and the asteroids live in the world registry, not in this
  // object. The other flavour draws its own walls, so leaving these behind
  // would double every wall on screen and feed the collision systems a second
  // set of obstacles. Called by SimulationModeSwitch before this object is
  // replaced, so the handles are still the ones it created.
  pain::Scene &scene = getScene();
  for (reg::Entity asteroid : m_asteroids)
    scene.removeEntity(asteroid);
  for (reg::Entity wall : m_walls)
    scene.removeEntity(wall);
  m_asteroids.clear();
  m_walls.clear();
}

void ParticleSimulationECS::setAsteroidCount(int count)
{
  m_asteroidCount = std::clamp(count, 0, Simulation::maxAsteroidCount);
  resetSimulation();
}

void ParticleSimulationECS::resetSimulation()
{
  pain::Scene &scene = getScene();

  // Match the entity list to the requested amount. Asteroids are the only
  // entities in the sprite+collider archetype this touches, so removing from
  // the back cannot disturb the walls, the camera or the mouse pointer.
  while (static_cast<int>(m_asteroids.size()) > m_asteroidCount) {
    scene.removeEntity(m_asteroids.back());
    m_asteroids.pop_back();
  }
  m_asteroids.reserve(static_cast<size_t>(m_asteroidCount));
  while (static_cast<int>(m_asteroids.size()) < m_asteroidCount) {
    size_t i = m_asteroids.size();
    m_asteroids.push_back(Asteroid::createECS(
        scene, *m_asteroidMaterials[i % m_asteroidMaterials.size()], {0.F, 0.F},
        {0.F, 0.F}, Simulation::asteroidRadius));
  }

  pain::RNG rng = {Simulation::asteroidSeed, 0, 0.4};
  for (int i = 0; i < m_asteroidCount; i++) {
    glm::vec2 pos{rng.gaussian<float>(), rng.gaussian<float>()};
    glm::vec2 vel{rng.gaussian<float>(), rng.gaussian<float>()};
    auto [transform, movement] =
        scene.getComponents<cmp::Pos2d, cmp::Mov2d>(m_asteroids[i]);
    transform.m_position = pos;
    movement.m_velocity = vel;
  }
}

void ParticleSimulationECS::renderControls()
{
  ImGui::Text("Asteroids: %d", static_cast<int>(m_asteroids.size()));
  if (ImGui::InputInt("Amount", &m_asteroidCount, 1,
                      Simulation::asteroidAmount))
    setAsteroidCount(m_asteroidCount);
  if (ImGui::Button("Reset"))
    resetSimulation();
}
