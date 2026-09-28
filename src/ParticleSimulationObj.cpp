#include "Assets/ManagerTexture.h"
#include "Asteroid.h"
#include "CustomPanel.h"
#include "Misc/Basic2dPlayer.h"
#include "MousePointer.h"
#include "ParticleSimulation.h"
#include "Wall.h"
#include <pain.h>
#include <painless.h>

#include "CoreRender/Renderer/RenderApi.h"
#include "Physics/Collision/ColDetection.h"
#include "Physics/Collision/ColReaction.h"
#include "imgui.h"
#include <algorithm>

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
          std::move(walls), assets.m_mousePointer,
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
    reg::Entity mp,                           //
    std::vector<pain::Material *> &&asteroidMaterials)
    : ParticleSimulationBase(entity, scene), m_mousePointer(std::move(mp)),
      m_orthoCamera(orthocamera), m_asteroids(std::move(asteroid)),
      m_walls(std::move(walls)),
      m_asteroidMaterials(std::move(asteroidMaterials)) {};

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

namespace
{
// =================================================================== //
// Non-ECS reimplementation of the two systems that drive the ECS branch:
// Render2dSys and NaiveCollisionSys.
//
// The rule kept throughout: same math, same order of operations, same
// branch structure. The only thing that changes is where the data lives
// (struct members in a std::vector instead of archetype chunks), which is
// exactly the thing being measured.
// =================================================================== //

/// Mirrors the shape-vs-shape tests in pain::ColDet, dispatched over the
/// collider variant exactly like the system does.
template <typename ShapeA, typename ShapeB>
pain::ColDet::Result detect(const glm::vec2 &centerA, const ShapeA &shapeA,
                            const glm::vec2 &centerB, const ShapeB &shapeB)
{
  using T1 = std::decay_t<ShapeA>;
  using T2 = std::decay_t<ShapeB>;
  if constexpr (std::is_same_v<T1, pain::AABBShape> &&
                std::is_same_v<T2, pain::AABBShape>) {
    return pain::ColDet::checkAABBCollision(centerA, shapeA.halfSize, centerB,
                                            shapeB.halfSize);
  } else if constexpr (std::is_same_v<T1, pain::CircleShape> &&
                       std::is_same_v<T2, pain::CircleShape>) {
    return pain::ColDet::checkCircleCollision(centerA, shapeA.radius, centerB,
                                              shapeB.radius);
  } else if constexpr (std::is_same_v<T1, pain::CircleShape> &&
                       std::is_same_v<T2, pain::AABBShape>) {
    // NOTE: if your LSP says centerA and centerB are swapped, ignore it
    return pain::ColDet::checkAABBCollisionCircle(centerB, shapeB.halfSize,
                                                  centerA, shapeA.radius);
  } else {
    return {false};
  }
}

/// Movable vs movable. Counterpart of
/// pain::Systems::detail::narrowPhaseCollision.
void narrowPhaseCollision(Asteroid::Object &a, Asteroid::Object &b)
{
  const glm::vec2 centerA = a.m_position + a.m_offset;
  const glm::vec2 centerB = b.m_position + b.m_offset;

  const auto hit = std::visit(
      [&](auto &&shapeA, auto &&shapeB) {
        return detect(centerA, shapeA, centerB, shapeB);
      },
      a.m_shape, b.m_shape);
  if (!hit.isDetected)
    return;

  if (a.m_isTrigger || b.m_isTrigger) {
    // The system would enqueue a CollisionEvent here. A plain object carries
    // no reg::Entity, so there is no identity to address that event to.
    // Every object in this simulation is built with m_isTrigger == false, so
    // this branch never runs for it.
    return;
  }
  pain::ColReaction::solidCollisionDynamic( //
      a.m_position,                         //
      a.m_velocity,                         //
      b.m_position,                         //
      b.m_velocity, hit.normal, hit.penetration);
}

/// Movable vs static. Counterpart of
/// pain::Systems::detail::narrowPhaseCollisionStatic. The static object is
/// never written to, only read.
void narrowPhaseCollisionStatic(Asteroid::Object &a, const Wall::Object &b)
{
  const glm::vec2 centerA = a.m_position + a.m_offset;
  const glm::vec2 centerB = b.m_position + b.m_offset;

  const auto hit = std::visit(
      [&](auto &&shapeA, auto &&shapeB) {
        return detect(centerA, shapeA, centerB, shapeB);
      },
      a.m_shape, b.m_shape);
  if (!hit.isDetected)
    return;

  if (a.m_isTrigger || b.m_isTrigger)
    return;
  pain::ColReaction::solidCollisionStatic( //
      a.m_position, a.m_velocity, b.m_position, hit.normal, hit.penetration);
}

/// Equivalent to Render2dSys.h
template <typename ObjectT, bool Rotated>
void submit(pain::Renderer2d &renderer2d, const ObjectT &obj)
{
  float rotation = 0.f;
  if constexpr (Rotated)
    rotation = obj.m_rotationRadians;
  std::visit(
      [&](auto &shape) {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (Rotated) {
          if constexpr (std::is_same_v<T, pain::QuadShape>) {
            renderer2d.submitQuad(obj.m_position, shape.side, rotation,
                                  obj.m_layer, *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::RectShape>) {
            renderer2d.submitRect(obj.m_position, shape.size, rotation,
                                  obj.m_layer, *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
            renderer2d.submitTri(obj.m_position, {shape.base, shape.height},
                                 rotation, obj.m_layer, *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::LineShape>) {
            renderer2d.submitLine(obj.m_position, shape.destination,
                                  shape.thickness, obj.m_layer,
                                  *obj.m_material);
          }
        } else {
          if constexpr (std::is_same_v<T, pain::QuadShape>) {
            renderer2d.submitQuad(obj.m_position, shape.side, obj.m_layer,
                                  *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::RectShape>) {
            renderer2d.submitRect(obj.m_position, shape.size, obj.m_layer,
                                  *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
            renderer2d.submitTri(obj.m_position, {shape.base, shape.height},
                                 obj.m_layer, *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::LineShape>) {
            renderer2d.submitLine(obj.m_position, shape.destination,
                                  shape.thickness, obj.m_layer,
                                  *obj.m_material);
          }
        }
      },
      obj.m_size);
}

} // namespace

// ======================================================================= //
// Non-ECS NaiveCollisionSys
// ======================================================================= //
void ParticleSimulationObj::onUpdate(pain::DeltaTime dt)
{
  // The two systems that do this work in the ECS branch run in this order
  // (see Sys::declareSystems): NaiveCollisionSys first, then KinematicsSys.
  // The order matters, so it is reproduced here: collisions are resolved
  // against the positions left over by the previous frame, and only then is
  // the new position integrated.
  {
    PROFILE_SCOPE("ParticleSimulationObj - collision vs moving objects");
    // One vector is one chunk, so the system collapses to the i < j half of
    // its chunk-vs-chunk loop: every pair is visited exactly once.
    const size_t count = m_asteroids.size();
    for (size_t i = 0; i < count; ++i) {
      for (size_t j = i + 1; j < count; ++j) {
        narrowPhaseCollision(m_asteroids[i], m_asteroids[j]);
      }
    }
  }
  {
    PROFILE_SCOPE("ParticleSimulationObj - collision vs static objects");
    for (auto &asteroid : m_asteroids) {
      for (const auto &wall : m_walls) {
        narrowPhaseCollisionStatic(asteroid, wall);
      }
    }
  }
  {
    PROFILE_SCOPE("ParticleSimulationObj - movement");
    // Non-ECS KinematicsSys. Walls have no movement, and nothing rotates the
    // asteroids' m_rotationRadians, so only the integration is left.
    const float dtf = dt.getSecondsf();
    for (auto &asteroid : m_asteroids) {
      asteroid.m_position += asteroid.m_velocity * dtf;
    }
  }
}

// ======================================================================= //
// Non-ECS Render2d
// ======================================================================= //
void ParticleSimulationObj::onRender(pain::RenderApi &rs,
                                     pain::DeltaTime currentTime)
{
  UNUSED(currentTime)
  PROFILE_FUNCTION();
  pain::Renderer2d &renderer2d = rs.m_renderer2d;
  {
    // The ECS asteroids own a RotationComponent, so the system reaches them
    // through its rotation query.
    PROFILE_SCOPE("ParticleSimulationObj::onRender - rotation quads");
    for (const auto &asteroid : m_asteroids)
      submit<decltype(asteroid), true>(renderer2d, asteroid);
  }
  {
    // The walls have no rotation member, so the system reaches them through
    // its texture query, which submits without the rotation argument.
    PROFILE_SCOPE("ParticleSimulationObj::onRender - texture quads");
    for (const auto &wall : m_walls)
      submit<decltype(wall), false>(renderer2d, wall);
  }
  // The Render2d system is still registered in this mode, so it keeps
  // rendering the ECS leftovers (camera grid, mouse pointer) and flushing the
  // RenderContext. Nothing to replicate here.
}
