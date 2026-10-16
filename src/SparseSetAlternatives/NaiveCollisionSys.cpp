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
void detail::narrowPhaseCollisionStatic( //
    Transform2dComponent &t_i,           //
    ColliderComponent &c_i,              //
    Movement2dComponent &m_i,            //
    reg::Entity entity_i,                //
    const Transform2dComponent &t_j,     //
    const ColliderComponent &c_j,        //
    reg::Entity entity_j,                //
    reg::EventDispatcher &eventDispatcher)
{
  glm::vec2 center1 = t_i.m_position + c_i.m_offset;
  glm::vec2 center2 = t_j.m_position + c_j.m_offset;
  ColDet::Result collisionHappened = {false};

  // -------------------------------------------------------------
  // Collision detection on movable vs static objects
  // -------------------------------------------------------------
  std::visit(
      [&](auto &&shape1, auto &&shape2) {
        using T1 = std::decay_t<decltype(shape1)>;
        using T2 = std::decay_t<decltype(shape2)>;
        if constexpr (std::is_same_v<T1, AABBShape> &&
                      std::is_same_v<T2, AABBShape>) {
          collisionHappened = ColDet::checkAABBCollision(
              center1, shape1.halfSize, center2, shape2.halfSize);
        } else if constexpr (std::is_same_v<T1, CircleShape> &&
                             std::is_same_v<T2, CircleShape>) {
          collisionHappened = ColDet::checkCircleCollision(
              center1, shape1.radius, center2, shape2.radius);
        } else if constexpr (std::is_same_v<T1, CircleShape> &&
                             std::is_same_v<T2, AABBShape>) {
          collisionHappened = ColDet::checkAABBCollisionCircle(
              center2, shape2.halfSize, center1, shape1.radius);
        }
      },
      c_i.m_shape, c_j.m_shape);

  // --- Perform Collision Reaction on Entities with Collision Scripts
  if (collisionHappened.isDetected) {
    if (c_i.m_isTrigger || c_j.m_isTrigger) {
      eventDispatcher.enqueue<CollisionEvent>({entity_i, entity_j,
                                               collisionHappened.normal,
                                               collisionHappened.penetration});
    } else {
      // PLOG_I("Static Collision between ({},{}) and ({},{})",
      //        TP_VEC2(t_i.m_position), TP_VEC2(t_j.m_position));
      ColReaction::solidCollisionStatic(t_i.m_position,
                                        m_i.m_velocity, //
                                        t_j.m_position, //
                                        collisionHappened.normal,
                                        collisionHappened.penetration);
    }
  }
}
void detail::narrowPhaseCollision( //
    Transform2dComponent &t_i,     //
    ColliderComponent &c_i,        //
    Movement2dComponent &m_i,      //
    reg::Entity entity_i,          //
    Transform2dComponent &t_j,     //
    ColliderComponent &c_j,        //
    Movement2dComponent &m_j,      //
    reg::Entity entity_j,          //
    reg::EventDispatcher &eventDispatcher)
{
  glm::vec2 center1 = t_i.m_position + c_i.m_offset;
  glm::vec2 center2 = t_j.m_position + c_j.m_offset;
  ColDet::Result collisionHappened = {false};

  // -------------------------------------------------------------
  // Collision detection on movable vs movable objects
  // -------------------------------------------------------------
  std::visit(
      [&](auto &&shape1, auto &&shape2) {
        using T1 = std::decay_t<decltype(shape1)>;
        using T2 = std::decay_t<decltype(shape2)>;
        if constexpr (std::is_same_v<T1, AABBShape> &&
                      std::is_same_v<T2, AABBShape>) {
          collisionHappened = ColDet::checkAABBCollision(
              center1, shape1.halfSize, center2, shape2.halfSize);
        } else if constexpr (std::is_same_v<T1, CircleShape> &&
                             std::is_same_v<T2, CircleShape>) {
          collisionHappened = ColDet::checkCircleCollision(
              center1, shape1.radius, center2, shape2.radius);
        } else if constexpr (std::is_same_v<T1, CircleShape> &&
                             std::is_same_v<T2, AABBShape>) {
          // NOTE: if your LSP says center2 and center1 are swapped, ignore it
          collisionHappened = ColDet::checkAABBCollisionCircle(
              center2, shape2.halfSize, center1, shape1.radius);
        }
      },
      c_i.m_shape, c_j.m_shape);

  // --- Perform Collision Reaction on Entities with Collision Scripts
  if (collisionHappened.isDetected) {
    if (c_i.m_isTrigger || c_j.m_isTrigger) {
      eventDispatcher.enqueue<CollisionEvent>( //
          {
              entity_i, entity_j,           //
              collisionHappened.normal,     //
              collisionHappened.penetration //
          });
    } else {
      ColReaction::solidCollisionDynamic( //
          t_i.m_position,                 //
          m_i.m_velocity,                 //
          t_j.m_position,                 //
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
  Transform2dComponent *transform;
  ColliderComponent *collider;
  Movement2dComponent *movement;
  reg::Entity entity;
};
struct StaticEntity {
  Transform2dComponent *transform;
  ColliderComponent *collider;
  reg::Entity entity;
};
} // namespace

void detail::_implOnUpdate(
    DeltaTime deltaTime,
    reg::View<Transform2dComponent, ColliderComponent> viewStatic,
    reg::View<Transform2dComponent, ColliderComponent, Movement2dComponent>
        viewDynamic,
    reg::EventDispatcher &eventDispatcher)
{
  UNUSED(deltaTime)

  // A View is a forEach-only cursor: it exposes no indices, no size, and cannot
  // be re-walked. Both pairwise passes below need random access (the moving
  // pass starts its inner loop at i + 1 to skip self comparisons and duplicate
  // unordered pairs), so each view is materialised once into a flat array.
  std::vector<DynamicEntity> dynamic;
  viewDynamic.forEach(
      [&](reg::Entity entity, Transform2dComponent &t, ColliderComponent &c,
          Movement2dComponent &m) { dynamic.push_back({&t, &c, &m, entity}); });

  std::vector<StaticEntity> statics;
  viewStatic.forEach(
      [&](reg::Entity entity, Transform2dComponent &t, ColliderComponent &c) {
        statics.push_back({&t, &c, entity});
      });

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
