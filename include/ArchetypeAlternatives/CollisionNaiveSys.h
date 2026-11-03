/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file CollisionNaiveSys.h
 * @brief Archetype alternative of the brute-force collision system.
 *
 * This is the archetype counterpart of
 * SparseSetAlternatives/CollisionNaiveSys.h. The original archetype system
 * walks its ChunkViews directly, while the sparse set version first flattens
 * every matching entity into flat vectors of pointers and then runs the
 * pairwise loops over those vectors.
 *
 * ArchAltNaiveCollisionSys keeps the archetype query, but flattens the
 * resulting chunks into the same dynamic and static vectors before the pair
 * loops. This makes the traversal, allocation and cache behaviour comparable to
 * the sparse set version instead of comparing two different iteration
 * strategies.
 *
 * @note Detection, reaction and event emission are the same detail functions
 * used by the regular archetype system.
 *
 * @note As with any system, the callback execution is only enabled because the
 * system inherits the corresponding interface:
 *  - IOnUpdate  → enables onUpdate() callbacks.
 */

#pragma once

#include "Assets/DeltaTime.h"
#include "Debugging/Profiling.h"
#include "ECS/Systems.h"
#include "Physics/Collision/Collider.h"
#include "Physics/MovementComponent.h"

namespace pain
{
namespace Systems
{

namespace archaltDetail
{
void _implOnUpdate(
    std::vector<reg::ChunkView<Transform2dComponent, ColliderComponent>>
        chunksStatic,
    std::vector<reg::ChunkView<Transform2dComponent, ColliderComponent,
                               Movement2dComponent>>
        chunks,
    reg::EventDispatcher &eventDispatcher);
} // namespace archaltDetail

/**
 * @brief Brute-force collision system with the sparse set traversal shape.
 *
 * Queries the archetype registry exactly like NaiveCollisionSys, but before the
 * pairwise passes it materialises the matching entities into flat vectors. The
 * two passes then iterate those vectors with the same indexed loops used by the
 * sparse set alternative.
 */
template <reg::CompileTimeBitMaskType CM>
struct ArchAltNaiveCollisionSys : public System<CM>, public IOnUpdate {
  /// @brief Component tags required by this system.
  using Tags = TypeList<Transform2dComponent, //
                        Movement2dComponent,  //
                        ColliderComponent>;

  /** @brief Inherit base System constructors. */
  using System<CM>::System;

  /** @brief Prevent default construction. */
  ArchAltNaiveCollisionSys() = delete;

  /**
   * @brief Flattens the archetype chunks and runs the naive collision passes.
   *
   * @param deltaTime Frame delta time.
   */
  void onUpdate(DeltaTime deltaTime) override
  {
    UNUSED(deltaTime);
    PROFILE_SCOPE("ArchAltNaiveCollisionSys - onUpdate")
    archaltDetail::_implOnUpdate(
        this->template query<Transform2dComponent, ColliderComponent>(
            exclude<Movement2dComponent>),
        this->template query<Transform2dComponent, ColliderComponent,
                             Movement2dComponent>(),
        this->m_eventDispatcher);
  }
};

} // namespace Systems
} // namespace pain
