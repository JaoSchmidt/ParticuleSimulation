/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file CollisionNaiveSys.h
 * @brief Archetype alternative of the brute-force collision system.
 *
 * This is the archetype mirror of ObjectAlternatives/CollisionNaiveSys.h: same
 * passes in the same order, and the same narrow phase code compiled the same
 * way. The comparison then measures the data layout instead of where the
 * functions happen to live, which is what separates it from the engine's
 * NaiveCollisionSys.
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
 * @brief Brute-force collision system mirroring the plain-object version.
 *
 * Queries the archetype registry exactly like NaiveCollisionSys, then runs the
 * same two pairwise passes as obj::Systems::NaiveCollision directly over the
 * chunk arrays.
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
   * @brief Runs the naive collision passes over the archetype chunks.
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
