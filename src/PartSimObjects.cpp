#include "Assets/ManagerTexture.h"
#include "Asteroid.h"
#include "CoreFiles/LogWrapper.h"
#include "Debugging/Profiling.h"
#include "ObjectAlternatives/CollisionNaiveSys.h"
#include "ObjectAlternatives/KinematicsSys.h"
#include "ObjectAlternatives/Render2dSys.h"
#include "ParticleSimulation.h"
#include "SimulationProfiling.h"
#include "Wall.h"
#include <pain.h>
#include <painless.h>

#include "CoreRender/Renderer/RenderApi.h"
#include "imgui.h"
#include <algorithm>
#include <string>

ParticleSimulationObj &ParticleSimulationObj::createScriptScene(
    pain::Scene &scene, const SimulationAssets &assets, int startAsteroidCount)
{
  std::vector<Wall::Object> walls;
  walls.reserve(assets.m_walls.size());
  for (const WallSpec &wall : assets.m_walls)
    walls.emplace_back(Wall::createObject(wall.m_position, wall.m_size,
                                          *assets.m_wallMaterial));

  std::vector<Asteroid::Object> asteroids;

  ParticleSimulationObj &script =
      pain::Scene::emplaceScript<ParticleSimulationObj>(
          scene.getEntity(), scene, assets.m_camera, std::move(asteroids),
          std::move(walls),
          std::vector<pain::Material *>(assets.m_asteroidMaterials));
  script.setAsteroidCount(startAsteroidCount);
  return script;
}

ParticleSimulationObj::ParticleSimulationObj(
    reg::Entity entity,                       //
    pain::Scene &scene,                       //
    reg::Entity orthocamera,                  //
    std::vector<Asteroid::Object> &&asteroid, //
    std::vector<Wall::Object> &&walls,        //
    std::vector<pain::Material *> &&asteroidMaterials)
    : pain::WorldObject(entity, scene),
      ParticleSimulationBase(orthocamera, std::move(asteroidMaterials)),
      m_eventDispatcher(scene.getEventDispatcher()),
      m_asteroids(std::move(asteroid)), m_walls(std::move(walls)) {};

void ParticleSimulationObj::setAsteroidCount(int count)
{
  m_asteroidCount = std::clamp(count, 0, Simulation::maxAsteroidCount);
  resetSimulation();
}

void ParticleSimulationObj::resetSimulation()
{
  m_asteroids.clear();
  m_asteroids.reserve(static_cast<size_t>(m_asteroidCount));
  pain::RNG rng = {Simulation::asteroidSeed, 0, 0.4};
  for (int i = 0; i < m_asteroidCount; i++) {
    glm::vec2 pos{rng.gaussian<float>(), rng.gaussian<float>()};
    glm::vec2 vel{rng.gaussian<float>(), rng.gaussian<float>()};
    pain::Material &m = *m_asteroidMaterials[static_cast<size_t>(i) %
                                             m_asteroidMaterials.size()];
    m_asteroids.emplace_back(
        Asteroid::createObject(m, pos, vel, Simulation::asteroidRadius));
  }
}

void ParticleSimulationObj::renderControls()
{
  ImGui::Text("Asteroids: %d", static_cast<int>(m_asteroids.size()));
  if (ImGui::InputInt("Amount", &m_asteroidCount, 1,
                      Simulation::asteroidAmount))
    setAsteroidCount(m_asteroidCount);
  if (ImGui::Button("Reset"))
    resetSimulation();
}

// ======================================================================= //
// Non-ECS NaiveCollisionSys + KinematicsSys
// ======================================================================= //
void ParticleSimulationObj::onUpdate(pain::DeltaTime dt)
{
#ifdef PROFILING
  if (m_profileFramesUntilStart == 0 && m_profileFramesUntilClose == 0)
    m_profileFramesUntilStart = SimulationProfiling::profileStartDelayFrames;

  if (m_profileFramesUntilStart > 0) {
    if (--m_profileFramesUntilStart == 0) {
      std::string availableResultFileName =
          SimulationProfiling::getNextAvailableFileName(
              SimulationProfiling::generateProfileFilename(
                  "ParticleSimulationObj", asteroidCount()));
      m_profileFramesUntilClose = SimulationProfiling::profileFrameCount;
      PLOG_T("Starting profile on file {} with {} frame(s)",
             availableResultFileName, m_profileFramesUntilClose);
      PROFILE_OPEN("ParticleSimulationObj", availableResultFileName.c_str());
    }
  } else if (m_profileFramesUntilClose > 0 &&
             --m_profileFramesUntilClose == 0) {
    PROFILE_CLOSE();
    resetWithDoubleAsteroids(*this);
    m_profileFramesUntilStart = SimulationProfiling::profileStartDelayFrames;
  }
#endif
  // The two systems that do this work in the ECS branch run in this order
  // (see Sys::declareSystems): NaiveCollisionSys first, then KinematicsSys.
  // The order matters, so it is reproduced here: collisions are resolved
  // against the positions left over by the previous frame, and only then is
  // the new position integrated.
  obj::Systems::NaiveCollision::onUpdate(m_asteroids, m_walls,
                                         m_eventDispatcher);
  obj::Systems::Kinematics::onUpdate(m_asteroids, dt);
}

// ======================================================================= //
// Non-ECS Render2d
// ======================================================================= //
void ParticleSimulationObj::onRender(pain::RenderApi &rs,
                                     pain::DeltaTime currentTime)
{
  UNUSED(currentTime)
  obj::Systems::Render2d::onRender(rs, m_asteroids, m_walls);
}
