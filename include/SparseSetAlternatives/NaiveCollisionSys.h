/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file NaiveCollisionSys.h
 * @brief Sparse set alternative of the archetype brute-force collision system.
 *
 * This is the same system as Physics/Collision/CollisionNaiveSys.h, ported from
 * the archetype registry to the sparse set registry. The component queries
 * change spelling:
 *
 *  - ArcheRegistry::query() returns a std::vector<reg::ChunkView<...>>, one
 *    view per archetype, and the caller indexes chunk.arrays by loop index.
 *  - SparseRegistry::view() returns a single reg::View<...> over one dense
 *    array per component, and the caller drives it with forEach().
 *
 * The archetype version's dynamic-vs-dynamic pass is a pairwise loop that needs
 * random access into the entity list (it starts the inner loop at i + 1 so an
 * entity is never compared against itself and each unordered pair is visited
 * exactly once). A View is a forEach-only cursor and exposes no indices, so the
 * pair loop is rebuilt by snapshotting each view once into a flat array of
 * pointers and then running the original indexed loop over that snapshot. The
 * pointers stay valid for the whole update: collision reaction only mutates
 * positions and velocities, and collision events are enqueued rather than
 * dispatched, so no component is added or removed and no dense array
 * reallocates.
 *
 * Everything else - detection, reaction, event emission and the guards around
 * them - is deliberately identical, so the two implementations can be compared
 * directly.
 *
 * @note Because a View iterates the smallest component pool rather than
 * archetype chunks, pairs are visited in a different order than in the
 * archetype version. For a brute-force solver that writes back velocities this
 * can change the exact numeric result, though both remain valid naive solvers.
 *
 * @note The class name, namespace, Tags and callbacks are kept identical to the
 * archetype version, so aliasesSystems.h needs no change when this header is
 * swapped in. Because both spellings define the same symbols, the archetype and
 * the sparse set variant must never be included in the same translation unit.
 *
 * @note As with any system, the callback execution is only enabled because the
 * system inherits the corresponding interface:
 *  - IOnUpdate  → enables onUpdate() callbacks.
 */

#pragma once

#include "Assets/DeltaTime.h"
#include "Debugging/Profiling.h"
#include "Physics/Collision/Collider.h"
#include "Physics/MovementComponent.h"
#include "SparseSetAlternatives/Systems.h"

namespace tcc
{
namespace Systems
{

namespace detail
{
void _implOnUpdate(
    DeltaTime deltaTime,
    reg::View<Transform2dComponent, ColliderComponent> viewStatic,
    reg::View<Transform2dComponent, ColliderComponent, Movement2dComponent>
        viewDynamic,
    reg::EventDispatcher &eventDispatcher);
void narrowPhaseCollisionStatic(          //
    Transform2dComponent &t_i,            //
    ColliderComponent &c_i,               //
    Movement2dComponent &m_i,             //
    reg::Entity entity_i,                 //
    const Transform2dComponent &t_j,      //
    const ColliderComponent &c_j,         //
    reg::Entity entity_j,                 //
    reg::EventDispatcher &eventDispatcher //
);

void narrowPhaseCollision(                //
    Transform2dComponent &t_i,            //
    ColliderComponent &c_i,               //
    Movement2dComponent &m_i,             //
    reg::Entity entity_i,                 //
    Transform2dComponent &t_j,            //
    ColliderComponent &c_j,               //
    Movement2dComponent &m_j,             //
    reg::Entity entity_j,                 //
    reg::EventDispatcher &eventDispatcher //
);

} // namespace detail

/**
 * @brief Brute-force collision detection system for dynamic and static
 * entities.
 *
 * NaiveCollisionSys iterates through all entities that contain the required
 * components and performs collision checks between every valid pair. Depending
 * on whether both entities are dynamic or one is static, different narrow-phase
 * resolution paths are executed.
 *
 * This system is primarily useful as:
 *  - A correctness reference for more optimized collision systems.
 *  - A lightweight solution for very small worlds.
 *  - A debugging tool during engine development.
 *
 * @note As with any system, the callback execution is only enabled because the
 * system inherits the corresponding interface:
 *  - IOnUpdate  → enables onUpdate() callbacks.
 *
 * @note The Tags list is validated at compile time to ensure that all required
 * components are registered in the component manager before this system can be
 * added to a scene.
 *
 * @see Transform2dComponent
 * @see Movement2dComponent
 * @see ColliderComponent
 * @see System
 * @see IOnUpdate
 */
template <reg::CompileTimeBitMaskType CM>
struct NaiveCollisionSys : public System<CM>, public IOnUpdate {
  /**
   * @brief Component tags required by this system.
   *
   * Declares that this system operates on entities containing:
   *  - Transform2dComponent
   *  - Movement2dComponent
   *  - ColliderComponent
   */
  using Tags = TypeList<Transform2dComponent, //
                        Movement2dComponent,  //
                        ColliderComponent>;

  /** @brief Inherit base System constructors. */
  using System<CM>::System;

  /** @brief Prevent default construction. */
  NaiveCollisionSys() = delete;

  /**
   * @brief Performs collision detection and resolution for the current frame.
   *
   * Iterates through all valid entity pairs and executes narrow-phase collision
   * tests and response logic.
   *
   * @param deltaTime Frame delta time.
   *
   * @note This method is invoked only because the system inherits from
   * IOnUpdate.
   */
  void onUpdate(DeltaTime deltaTime) override
  {
    PROFILE_SCOPE("CollisionNaiveSys - onUpdate")
    detail::_implOnUpdate(
        deltaTime,
        this->template view<Transform2dComponent, ColliderComponent>(
            exclude<Movement2dComponent>),
        this->template view<Transform2dComponent, ColliderComponent,
                            Movement2dComponent>(),
        this->m_eventDispatcher);
  }
};

} // namespace Systems
} // namespace tcc
