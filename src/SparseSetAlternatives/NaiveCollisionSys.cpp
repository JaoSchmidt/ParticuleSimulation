/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "SparseSetAlternatives/NaiveCollisionSys.h"

#include "Debugging/Profiling.h"
#include "Misc/Events.h"
#include "Physics/Collision/ColDetection.h"
#include "Physics/Collision/ColReaction.h"
#include "Physics/Collision/Collider.h"
namespace tcc
{
namespace Systems
{

// =============================================================
// COLLISION DETECTION
// =============================================================
void detail::narrowPhaseCollisionStatic(   //
    pain::Transform2dComponent &t_i,       //
    pain::ColliderComponent &c_i,          //
    pain::Movement2dComponent &m_i,        //
    reg::Entity entity_i,                  //
    const pain::Transform2dComponent &t_j, //
    const pain::ColliderComponent &c_j,    //
    reg::Entity entity_j,                  //
    reg::EventDispatcher &eventDispatcher)
{
  glm::vec2 center1 = t_i.m_position + c_i.m_offset;
  glm::vec2 center2 = t_j.m_position + c_j.m_offset;
  pain::ColDet::Result collisionHappened = {false};

  // -------------------------------------------------------------
  // Collision detection on movable vs static objects
  // -------------------------------------------------------------
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
      c_i.m_shape, c_j.m_shape);

  // --- Perform Collision Reaction on Entities with Collision Scripts
  if (collisionHappened.isDetected) {
    if (c_i.m_isTrigger || c_j.m_isTrigger) {
      eventDispatcher.enqueue<pain::CollisionEvent>(
          {entity_i, entity_j, collisionHappened.normal,
           collisionHappened.penetration});
    } else {
      // PLOG_I("Static Collision between ({},{}) and ({},{})",
      //        TP_VEC2(t_i.m_position), TP_VEC2(t_j.m_position));
      pain::ColReaction::solidCollisionStatic(t_i.m_position,
                                              m_i.m_velocity, //
                                              t_j.m_position, //
                                              collisionHappened.normal,
                                              collisionHappened.penetration);
    }
  }
}
void detail::narrowPhaseCollision(   //
    pain::Transform2dComponent &t_i, //
    pain::ColliderComponent &c_i,    //
    pain::Movement2dComponent &m_i,  //
    reg::Entity entity_i,            //
    pain::Transform2dComponent &t_j, //
    pain::ColliderComponent &c_j,    //
    pain::Movement2dComponent &m_j,  //
    reg::Entity entity_j,            //
    reg::EventDispatcher &eventDispatcher)
{
  glm::vec2 center1 = t_i.m_position + c_i.m_offset;
  glm::vec2 center2 = t_j.m_position + c_j.m_offset;
  pain::ColDet::Result collisionHappened = {false};

  // -------------------------------------------------------------
  // Collision detection on movable vs movable objects
  // -------------------------------------------------------------
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
      c_i.m_shape, c_j.m_shape);

  // --- Perform Collision Reaction on Entities with Collision Scripts
  if (collisionHappened.isDetected) {
    if (c_i.m_isTrigger || c_j.m_isTrigger) {
      eventDispatcher.enqueue<pain::CollisionEvent>( //
          {
              entity_i, entity_j,           //
              collisionHappened.normal,     //
              collisionHappened.penetration //
          });
    } else {
      pain::ColReaction::solidCollisionDynamic( //
          t_i.m_position,                       //
          m_i.m_velocity,                       //
          t_j.m_position,                       //
          m_j.m_velocity, collisionHappened.normal,
          collisionHappened.penetration);
    }
  }
}

namespace
{
// Flattened handle to one entity of a sparse set view. The pointers borrow
// memory owned by the registry's dense arrays, which are not touched for the
// duration of the update.
struct DynamicEntity {
  pain::Transform2dComponent *transform;
  pain::ColliderComponent *collider;
  pain::Movement2dComponent *movement;
  reg::Entity entity;
};
struct StaticEntity {
  pain::Transform2dComponent *transform;
  pain::ColliderComponent *collider;
  reg::Entity entity;
};
} // namespace

void detail::_implOnUpdate(
    pain::DeltaTime deltaTime,
    reg::View<pain::Transform2dComponent, pain::ColliderComponent> viewStatic,
    reg::View<pain::Transform2dComponent, pain::ColliderComponent,
              pain::Movement2dComponent>
        viewDynamic,
    reg::EventDispatcher &eventDispatcher)
{
  UNUSED(deltaTime)

  // A View is a forEach-only cursor: it exposes no indices, no size, and cannot
  // be re-walked. Both pairwise passes below need random access (the moving
  // pass starts its inner loop at i + 1 to skip self comparisons and duplicate
  // unordered pairs), so each view is materialised once into a flat array.
  std::vector<DynamicEntity> dynamic;
  viewDynamic.forEach([&](reg::Entity entity, pain::Transform2dComponent &t,
                          pain::ColliderComponent &c,
                          pain::Movement2dComponent &m) {
    dynamic.push_back({&t, &c, &m, entity});
  });

  std::vector<StaticEntity> statics;
  viewStatic.forEach(
      [&](reg::Entity entity, pain::Transform2dComponent &t,
          pain::ColliderComponent &c) { statics.push_back({&t, &c, entity}); });

  {
    PROFILE_SCOPE(
        "Scene::updateSystems - collision detection vs moving objects");
    for (size_t i = 0; i < dynamic.size(); ++i) {
      // NOTE: start at i + 1 to avoid an entity colliding with itself and to
      // avoid testing each unordered pair twice.
      for (size_t j = i + 1; j < dynamic.size(); ++j) {
        narrowPhaseCollision(      //
            *dynamic[i].transform, //
            *dynamic[i].collider,  //
            *dynamic[i].movement,  //
            dynamic[i].entity,     //
            *dynamic[j].transform, //
            *dynamic[j].collider,  //
            *dynamic[j].movement,  //
            dynamic[j].entity,     //
            eventDispatcher);
      }
    }
  }
  {
    PROFILE_SCOPE(
        "Scene::updateSystems - collision detection vs static objects");

    for (size_t i = 0; i < dynamic.size(); ++i) {
      for (size_t j = 0; j < statics.size(); ++j) {
        narrowPhaseCollisionStatic( //
            *dynamic[i].transform,  //
            *dynamic[i].collider,   //
            *dynamic[i].movement,   //
            dynamic[i].entity,      //
            *statics[j].transform,  //
            *statics[j].collider,   //
            statics[j].entity,      //
            eventDispatcher);
      }
    }
  }
}

} // namespace Systems

} // namespace tcc
