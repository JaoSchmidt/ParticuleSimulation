/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file KinematicsSys.h
 * @brief Sparse set alternative of the archetype Kinematics system.
 *
 * This is the same system as Physics/KinematicsSys.h, ported from the
 * archetype registry to the sparse set registry. The only structural change is
 * how entities are enumerated:
 *
 *  - ArcheRegistry::query() returns a std::vector<reg::ChunkView<...>>, one
 *    view per archetype, and the caller walks chunk.arrays/SoA columns by
 *    index.
 *  - SparseRegistry::view() returns a single reg::View<...> over one dense
 *    array per component, and the caller hands a lambda to forEach().
 *
 * Everything else - which components are touched, in which order, with which
 * math - is deliberately identical, so the two implementations can be compared
 * directly.
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
#include "ECS/Components/ComponentManager.h"
#include "Physics/Movement3dComponent.h"
#include "Physics/MovementComponent.h"
#include "Physics/RotationComponent.h"
#include "SparseSetAlternatives/Systems.h"

namespace tcc
{
namespace Systems
{

namespace detail
{
/**
 * @brief Body of the update, expressed over sparse set views.
 *
 * The archetype variant receives one ChunkView per archetype; here each
 * parameter is the single View covering every entity owning those components,
 * and the work moves into the lambda passed to View::forEach.
 */
void _implOnUpdate(
    DeltaTime deltaTime, reg::View<RotationComponent> viewRotation,
    reg::View<Transform2dComponent, Movement2dComponent> viewMovement2d,
    reg::View<Transform3dComponent, Movement3dComponent> viewMovement3d);
} // namespace detail

/**
 * @brief Performs per-frame kinematic integration and rotation updates.
 *
 * Same duties as the archetype Kinematics system:
 *  - Rotation vector updates based on RotationComponent::m_rotationRadians.
 *  - Position integration using velocity and frame delta time.
 *
 * The system operates entirely during the update phase and does not participate
 * in rendering or event processing.
 *
 * @note As with any system, the callback execution is only enabled because the
 * system inherits the corresponding interface:
 *  - IOnUpdate  → enables onUpdate() callbacks.
 *
 * @see Transform2dComponent
 * @see Movement2dComponent
 * @see RotationComponent
 * @see System
 * @see IOnUpdate
 */
template <reg::CompileTimeBitMaskType CM>
struct Kinematics : public System<CM>, IOnUpdate {
  /**
   * @brief Component tags required by this system.
   *
   * Declares that this system operates on entities containing:
   *  - Transform2dComponent
   *  - Movement2dComponent
   *
   * RotationComponent is queried independently during update.
   */
  using Tags = TypeList<Transform2dComponent, Movement2dComponent>;

  /** @brief Inherit base System constructors. */
  using System<CM>::System;

  /** @brief Prevent default construction. */
  Kinematics() = delete;

  /**
   * @brief Performs kinematic updates for the current frame.
   *
   * Builds the three views and hands them to the shared implementation. The
   * views are built here and discarded after the update: a View is ephemeral
   * and only borrows the registry's records, so it must not outlive the frame.
   *
   * @param deltaTime Frame delta time.
   *
   * @note This method is invoked only because the system inherits from
   * IOnUpdate.
   */
  void onUpdate(DeltaTime deltaTime) override
  {
    detail::_implOnUpdate(
        deltaTime, this->template view<RotationComponent>(),
        this->template view<Transform2dComponent, Movement2dComponent>(),
        this->template view<Transform3dComponent, Movement3dComponent>());
  }
};

} // namespace Systems
} // namespace tcc
