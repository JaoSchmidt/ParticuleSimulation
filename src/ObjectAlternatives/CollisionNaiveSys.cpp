/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ObjectAlternatives/CollisionNaiveSys.h"

#include "Debugging/Profiling.h"
#include "Physics/Collision/ColDetection.h"
#include "Physics/Collision/ColReaction.h"

namespace obj::Systems
{

namespace
{
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
} // namespace

void NaiveCollision::onUpdate(std::vector<Asteroid::Object> &asteroids,
                              const std::vector<Wall::Object> &walls)
{
  {
    PROFILE_SCOPE("ParticleSimulationObj - collision vs moving objects");
    // One vector is one chunk, so the system collapses to the i < j half of
    // its chunk-vs-chunk loop: every pair is visited exactly once.
    const size_t count = asteroids.size();
    for (size_t i = 0; i < count; ++i) {
      for (size_t j = i + 1; j < count; ++j) {
        narrowPhaseCollision(asteroids[i], asteroids[j]);
      }
    }
  }
  {
    PROFILE_SCOPE("ParticleSimulationObj - collision vs static objects");
    for (auto &asteroid : asteroids) {
      for (const auto &wall : walls) {
        narrowPhaseCollisionStatic(asteroid, wall);
      }
    }
  }
}

} // namespace obj::Systems
