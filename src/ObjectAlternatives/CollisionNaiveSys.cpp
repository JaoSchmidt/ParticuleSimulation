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
/// Counterpart of pain::Systems::detail::narrowPhaseCollision and
/// narrowPhaseCollisionStatic. @p a is always movable, and a static @p b has no
/// velocity and is never written to.
void narrowPhaseCollision(IObject &a, IObject &b,
                          reg::EventDispatcher &eventDispatcher)
{
  glm::vec2 center1 = a.position() + a.offset();
  glm::vec2 center2 = b.position() + b.offset();
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
      a.shape(), b.shape());

  if (collisionHappened.isDetected) {
    if (a.isTrigger() || b.isTrigger()) {
      // HACK: ED isn't actually implemented for object simulation, but it's
      // kept for a more authentic comparasion with ECS
      eventDispatcher.enqueue<pain::CollisionEvent>( //
          {
              reg::Entity{0}, reg::Entity{0}, //
              collisionHappened.normal,       //
              collisionHappened.penetration   //
          });
    } else if (glm::vec2 *velocityB = b.velocity()) {
      pain::ColReaction::solidCollisionDynamic( //
          a.position(),                         //
          *a.velocity(),                        //
          b.position(),                         //
          *velocityB,                           //
          collisionHappened.normal,             //
          collisionHappened.penetration);
    } else {
      pain::ColReaction::solidCollisionStatic( //
          a.position(),                        //
          *a.velocity(),                       //
          b.position(),                        //
          collisionHappened.normal,            //
          collisionHappened.penetration);
    }
  }
}
} // namespace

void NaiveCollision::onUpdate(ObjectList &objects, size_t staticCount,
                              reg::EventDispatcher &eventDispatcher)
{
  PROFILE_SCOPE("Scene::updateSystems - collision detection")
  const size_t count = objects.size();
  {
    PROFILE_SCOPE("ParticleSimulationObj - collision vs moving objects");
    // Every movable pair is visited exactly once
    for (size_t i = staticCount; i < count; ++i) {
      for (size_t j = i + 1; j < count; ++j) {
        narrowPhaseCollision(*objects[i], *objects[j], eventDispatcher);
      }
    }
  }
  {
    PROFILE_SCOPE("ParticleSimulationObj - collision vs static objects");
    for (size_t i = staticCount; i < count; ++i) {
      for (size_t j = 0; j < staticCount; ++j) {
        narrowPhaseCollision(*objects[i], *objects[j], eventDispatcher);
      }
    }
  }
}
} // namespace obj::Systems
