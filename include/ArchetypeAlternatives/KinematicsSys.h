/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file KinematicsSys.h
 * @brief Archetype alternative of the engine's kinematics system.
 *
 * This is the mirror of Physics/KinematicsSys.h, compiled inside the game
 * instead of the engine: same queries, same passes and same profiling scopes.
 *
 * @note As with any system, the callback execution is only enabled because the
 * system inherits the corresponding interface:
 *  - IOnUpdate  → enables onUpdate() callbacks.
 */

#pragma once

#include "Assets/DeltaTime.h"
#include "Debugging/Profiling.h"
#include "ECS/Components/ComponentManager.h"
#include "ECS/Systems.h"
#include "Physics/Movement3dComponent.h"
#include "Physics/MovementComponent.h"
#include "Physics/RotationComponent.h"

namespace tcc::Systems
{

namespace detail
{
void _implOnUpdate(
    pain::DeltaTime deltaTime,
    std::vector<reg::ChunkView<pain::RotationComponent>> chunksRotation,
    std::vector<
        reg::ChunkView<pain::Transform2dComponent, pain::Movement2dComponent>>
        chunksMovement2d,
    std::vector<
        reg::ChunkView<pain::Transform3dComponent, pain::Movement3dComponent>>
        chunksMovement3d);
} // namespace detail

/**
 * @brief Performs per-frame kinematic integration and rotation updates.
 *
 * Mirrors pain::Systems::Kinematics:
 *  - Rotation vector updates based on RotationComponent::m_rotationRadians.
 *  - Position integration using velocity and frame delta time.
 *
 * @see pain::Systems::Kinematics
 */
template <reg::CompileTimeBitMaskType CM>
struct AltKinematics : public pain::System<CM>, pain::IOnUpdate {
  /**
   * @brief Component tags required by this system.
   *
   * RotationComponent is queried independently during update.
   */
  using Tags =
      pain::TypeList<pain::Transform2dComponent, pain::Movement2dComponent>;

  /** @brief Inherit base System constructors. */
  using pain::System<CM>::System;

  /** @brief Prevent default construction. */
  AltKinematics() = delete;

  /**
   * @brief Performs kinematic updates for the current frame.
   *
   * @param deltaTime Frame delta time.
   */
  void onUpdate(pain::DeltaTime deltaTime) override
  {
    PROFILE_SCOPE("Scene::udpateSystems - Kinematics");
    detail::_implOnUpdate(deltaTime,
                          this->template query<pain::RotationComponent>(),
                          this->template query<pain::Transform2dComponent,
                                               pain::Movement2dComponent>(),
                          this->template query<pain::Transform3dComponent,
                                               pain::Movement3dComponent>());
  }
};

} // namespace tcc::Systems
