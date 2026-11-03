#include "Assets/ManagerTexture.h"
#include "Asteroid.h"
#include "CoreFiles/LogWrapper.h"
#include "Debugging/Profiling.h"
#include "Misc/Basic2dPlayer.h"
#include "MousePointer.h"
#include "ParticleSimulation.h"
#include "SimulationProfiling.h"
#include "Wall.h"
#include "imgui.h"
#include <algorithm>
#include <pain.h>
#include <painless.h>
#include <string>

ParticleSimulationArchetype &ParticleSimulationArchetype::createScriptScene(
    pain::Scene &scene, const SimulationAssets &assets, int startAsteroidCount)
{
  // Only the walls are built here, as entities. The camera, the mouse pointer,
  // the textures and the materials are shared with the other flavour and
  // arrive through @p assets, and the asteroids come from resetSimulation().
  std::vector<reg::Entity> walls;
  walls.reserve(assets.m_walls.size());
  for (const WallSpec &wall : assets.m_walls)
    walls.emplace_back(Wall::createECS(scene, wall.m_position, wall.m_size,
                                       *assets.m_wallMaterial));

  std::vector<reg::Entity> asteroids;

  ParticleSimulationArchetype &script =
      pain::Scene::emplaceScript<ParticleSimulationArchetype>(
          scene.getEntity(), scene, assets.m_camera, std::move(asteroids),
          std::move(walls),
          std::vector<pain::Material *>(assets.m_asteroidMaterials));
  script.setAsteroidCount(startAsteroidCount);
  return script;
}

ParticleSimulationArchetype::ParticleSimulationArchetype(
    reg::Entity entity,                  //
    pain::Scene &scene,                  //
    reg::Entity orthocamera,             //
    std::vector<reg::Entity> &&asteroid, //
    std::vector<reg::Entity> &&walls,    //
    std::vector<pain::Material *> &&asteroidMaterials)
    : pain::WorldObject(entity, scene),
      ParticleSimulationBase(orthocamera, std::move(asteroidMaterials)),
      m_asteroids(std::move(asteroid)), m_walls(std::move(walls)) {};

void ParticleSimulationArchetype::setAsteroidCount(int count)
{
  m_asteroidCount = std::clamp(count, 0, Simulation::maxAsteroidCount);
  resetSimulation();
}

void ParticleSimulationArchetype::resetSimulation()
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

void ParticleSimulationArchetype::renderControls()
{
  ImGui::Text("Asteroids: %d", static_cast<int>(m_asteroids.size()));
  if (ImGui::InputInt("Amount", &m_asteroidCount, 1,
                      Simulation::asteroidAmount))
    setAsteroidCount(m_asteroidCount);
  if (ImGui::Button("Reset"))
    resetSimulation();
}

void ParticleSimulationArchetype::onUpdate(pain::DeltaTime dt)
{
  UNUSED(dt)
#ifdef PROFILING
  if (m_profileFramesUntilStart == 0 && m_profileFramesUntilClose == 0)
    m_profileFramesUntilStart = SimulationProfiling::profileStartDelayFrames;

  if (m_profileFramesUntilStart > 0) {
    if (--m_profileFramesUntilStart == 0) {
      std::string availableResultFileName =
          SimulationProfiling::getNextAvailableFileName(
              SimulationProfiling::generateProfileFilename(
                  "ParticleSimulationArchetype", asteroidCount()));
      m_profileFramesUntilClose = SimulationProfiling::profileFrameCount;
      PLOG_T("Starting profile on file {} with {} frame(s)",
             availableResultFileName, m_profileFramesUntilClose);
      PROFILE_OPEN("ParticleSimulationArchetype",
                   availableResultFileName.c_str());
    }
  } else if (m_profileFramesUntilClose > 0 &&
             --m_profileFramesUntilClose == 0) {
    PROFILE_CLOSE();
    resetWithDoubleAsteroids(*this);
    m_profileFramesUntilStart = SimulationProfiling::profileStartDelayFrames;
  }
#endif
}
