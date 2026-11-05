/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ArchetypeAlternatives/CollisionNaiveSys.h"

#include "Debugging/Profiling.h"
#include "Misc/Events.h"
#include "Physics/Collision/ColDetection.h"
#include "Physics/Collision/ColReaction.h"

namespace pain
{
namespace Systems
{

namespace
{
/// Movable vs movable. Counterpart of the narrowPhaseCollision in
/// ObjectAlternatives/CollisionNaiveSys.cpp. The entities are references so
/// they are only read when an event is actually raised.
inline void
narrowPhaseCollision(Transform2dComponent &t_a, const ColliderComponent &c_a,
                     Movement2dComponent &m_a, const reg::Entity &e_a,
                     Transform2dComponent &t_b, const ColliderComponent &c_b,
                     Movement2dComponent &m_b, const reg::Entity &e_b,
                     reg::EventDispatcher &eventDispatcher)
{
  glm::vec2 center1 = t_a.m_position + c_a.m_offset;
  glm::vec2 center2 = t_b.m_position + c_b.m_offset;
  ColDet::Result collisionHappened = {false};

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
      c_a.m_shape, c_b.m_shape);

  if (collisionHappened.isDetected) {
    if (c_a.m_isTrigger || c_b.m_isTrigger) {
      eventDispatcher.enqueue<CollisionEvent>( //
          {
              e_a, e_b,                     //
              collisionHappened.normal,     //
              collisionHappened.penetration //
          });
    } else {
      ColReaction::solidCollisionDynamic( //
          t_a.m_position,                 //
          m_a.m_velocity,                 //
          t_b.m_position,                 //
          m_b.m_velocity,                 //
          collisionHappened.normal,       //
          collisionHappened.penetration);
    }
  }
}

/// Movable vs static. Counterpart of the narrowPhaseCollisionStatic in
/// ObjectAlternatives/CollisionNaiveSys.cpp. The static object is never written
/// to, only read.
inline void narrowPhaseCollisionStatic(
    Transform2dComponent &t_a, const ColliderComponent &c_a,
    Movement2dComponent &m_a, const reg::Entity &e_a,
    const Transform2dComponent &t_b, const ColliderComponent &c_b,
    const reg::Entity &e_b, reg::EventDispatcher &eventDispatcher)
{
  glm::vec2 center1 = t_a.m_position + c_a.m_offset;
  glm::vec2 center2 = t_b.m_position + c_b.m_offset;
  ColDet::Result collisionHappened = {false};

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
      c_a.m_shape, c_b.m_shape);

  if (collisionHappened.isDetected) {
    if (c_a.m_isTrigger || c_b.m_isTrigger) {
      eventDispatcher.enqueue<CollisionEvent>( //
          {
              e_a, e_b,                     //
              collisionHappened.normal,     //
              collisionHappened.penetration //
          });
    } else {
      ColReaction::solidCollisionStatic( //
          t_a.m_position,                //
          m_a.m_velocity,                //
          t_b.m_position,                //
          collisionHappened.normal,      //
          collisionHappened.penetration);
    }
  }
}
} // namespace

void archaltDetail::_implOnUpdate(
    std::vector<reg::ChunkView<Transform2dComponent, ColliderComponent>>
        chunksStatic,
    std::vector<reg::ChunkView<Transform2dComponent, ColliderComponent,
                               Movement2dComponent>>
        chunks,
    reg::EventDispatcher &eventDispatcher)
{
  PROFILE_SCOPE("Scene::updateSystems - collision detection")
  {
    PROFILE_SCOPE(
        "Scene::updateSystems - collision detection vs moving objects");
    // With a single chunk this is the object version's i < j loop. Every chunk
    // pair is only needed because a query can match several archetypes.
    for (size_t ci = 0; ci < chunks.size(); ++ci) {
      auto [t_i, c_i, m_i] = chunks[ci].arrays;
      const reg::Entity *e_i = chunks[ci].entities.data();
      const size_t count_i = chunks[ci].count;

      for (size_t cj = ci; cj < chunks.size(); ++cj) {
        auto [t_j, c_j, m_j] = chunks[cj].arrays;
        const reg::Entity *e_j = chunks[cj].entities.data();
        const size_t count_j = chunks[cj].count;
        const bool same_chunk = (ci == cj);

        for (size_t i = 0; i < count_i; ++i) {
          for (size_t j = same_chunk ? i + 1 : 0; j < count_j; ++j) {
            narrowPhaseCollision(t_i[i], c_i[i], m_i[i], e_i[i], //
                                 t_j[j], c_j[j], m_j[j], e_j[j], //
                                 eventDispatcher);
          }
        }
      }
    }
  }
  {
    PROFILE_SCOPE(
        "Scene::updateSystems - collision detection vs static objects");
    for (auto &chunk : chunks) {
      auto [t_i, c_i, m_i] = chunk.arrays;
      const reg::Entity *e_i = chunk.entities.data();

      for (auto &chunkStatic : chunksStatic) {
        auto [t_j, c_j] = chunkStatic.arrays;
        const reg::Entity *e_j = chunkStatic.entities.data();

        for (size_t i = 0; i < chunk.count; ++i) {
          for (size_t j = 0; j < chunkStatic.count; ++j) {
            narrowPhaseCollisionStatic(t_i[i], c_i[i], m_i[i], e_i[i], //
                                       t_j[j], c_j[j], e_j[j],         //
                                       eventDispatcher);
          }
        }
      }
    }
  }
}

} // namespace Systems
} // namespace pain
