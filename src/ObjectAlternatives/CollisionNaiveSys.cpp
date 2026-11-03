/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ObjectAlternatives/CollisionNaiveSys.h"

#include "Debugging/Profiling.h"
#include "Misc/Events.h"
#include "Physics/Collision/ColDetection.h"
#include "Physics/Collision/ColReaction.h"

namespace obj::Systems
{

namespace
{
/// Movable vs movable. Counterpart of
/// pain::Systems::detail::narrowPhaseCollision.
void narrowPhaseCollision(Asteroid::Object &a, Asteroid::Object &b,
                          reg::EventDispatcher &eventDispatcher)
{
  glm::vec2 center1 = a.m_position + a.m_offset;
  glm::vec2 center2 = b.m_position + b.m_offset;
  pain::ColDet::Result collisionHappened = {false};

  std::visit(
      [&](auto &&shape1, auto &&shape2) {
        using T1 = std::decay_t<decltype(shape1)>;
        using T2 = std::decay_t<decltype(shape2)>;
        if constexpr (std::is_same_v<T1, pain::AABBShape> &&
                      std::is_same_v<T2, pain::AABBShape>) {
          collisionHappened = pain::ColDet::checkAABBCollision(
              center1, shape1.halfSize, center2, shape2.halfSize);
        } else if constexpr (std::is_same_v<T1, pain::CircleShape> &&
                             std::is_same_v<T2, pain::CircleShape>) {
          collisionHappened = pain::ColDet::checkCircleCollision(
              center1, shape1.radius, center2, shape2.radius);
        } else if constexpr (std::is_same_v<T1, pain::CircleShape> &&
                             std::is_same_v<T2, pain::AABBShape>) {
          // NOTE: if your LSP says center2 and center1 are swapped, ignore it
          collisionHappened = pain::ColDet::checkAABBCollisionCircle(
              center2, shape2.halfSize, center1, shape1.radius);
        }
      },
      a.m_shape, b.m_shape);

  if (collisionHappened.isDetected) {
    if (a.m_isTrigger || b.m_isTrigger) {
      // HACK: ED isn't actually implemented for object simulation, but it's
      // kept for a more authentic comparasion with ECS
      eventDispatcher.enqueue<pain::CollisionEvent>( //
          {
              reg::Entity{0}, reg::Entity{0}, //
              collisionHappened.normal,       //
              collisionHappened.penetration   //
          });
    } else {
      pain::ColReaction::solidCollisionDynamic( //
          a.m_position,                         //
          a.m_velocity,                         //
          b.m_position,                         //
          b.m_velocity,                         //
          collisionHappened.normal,             //
          collisionHappened.penetration);
    }
  }
}

/// Movable vs static. Counterpart of
/// pain::Systems::detail::narrowPhaseCollisionStatic. The static object is
/// never written to, only read.
void narrowPhaseCollisionStatic(Asteroid::Object &a, const Wall::Object &b,
                                reg::EventDispatcher &eventDispatcher)
{
  glm::vec2 center1 = a.m_position + a.m_offset;
  glm::vec2 center2 = b.m_position + b.m_offset;
  pain::ColDet::Result collisionHappened = {false};

  std::visit(
      [&](auto &&shape1, auto &&shape2) {
        using T1 = std::decay_t<decltype(shape1)>;
        using T2 = std::decay_t<decltype(shape2)>;
        if constexpr (std::is_same_v<T1, pain::AABBShape> &&
                      std::is_same_v<T2, pain::AABBShape>) {
          collisionHappened = pain::ColDet::checkAABBCollision(
              center1, shape1.halfSize, center2, shape2.halfSize);
        } else if constexpr (std::is_same_v<T1, pain::CircleShape> &&
                             std::is_same_v<T2, pain::CircleShape>) {
          collisionHappened = pain::ColDet::checkCircleCollision(
              center1, shape1.radius, center2, shape2.radius);
        } else if constexpr (std::is_same_v<T1, pain::CircleShape> &&
                             std::is_same_v<T2, pain::AABBShape>) {
          collisionHappened = pain::ColDet::checkAABBCollisionCircle(
              center2, shape2.halfSize, center1, shape1.radius);
        }
      },
      a.m_shape, b.m_shape);

  if (collisionHappened.isDetected) {
    if (a.m_isTrigger || b.m_isTrigger) {
      // HACK: ED isn't actually implemented for object simulation, but it's
      // kept for a more authentic comparasion with ECS
      eventDispatcher.enqueue<pain::CollisionEvent>( //
          {
              reg::Entity{0}, reg::Entity{0}, //
              collisionHappened.normal,       //
              collisionHappened.penetration   //
          });
    } else {
      pain::ColReaction::solidCollisionStatic( //
          a.m_position,                        //
          a.m_velocity,                        //
          b.m_position,                        //
          collisionHappened.normal,            //
          collisionHappened.penetration);
    }
  }
}
} // namespace

void NaiveCollision::onUpdate(std::vector<Asteroid::Object> &asteroids,
                              const std::vector<Wall::Object> &walls,
                              reg::EventDispatcher &eventDispatcher)
{
  PROFILE_SCOPE("Scene::updateSystems - collision detection")
  {
    PROFILE_SCOPE("ParticleSimulationObj - collision vs moving objects");
    // One vector is one chunk, so the system collapses to the i < j half of
    // its chunk-vs-chunk loop: every pair is visited exactly once.
    const size_t count = asteroids.size();
    for (size_t i = 0; i < count; ++i) {
      for (size_t j = i + 1; j < count; ++j) {
        narrowPhaseCollision(asteroids[i], asteroids[j], eventDispatcher);
      }
    }
  }
  {
    PROFILE_SCOPE("ParticleSimulationObj - collision vs static objects");
    for (auto &asteroid : asteroids) {
      for (const auto &wall : walls) {
        narrowPhaseCollisionStatic(asteroid, wall, eventDispatcher);
      }
    }
  }
}
} // namespace obj::Systems
