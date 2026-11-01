/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file KinematicsSys.h
 * @brief Plain-object equivalent of the ECS kinematics system.
 *
 * The plain-object simulation stores everything an asteroid needs inline in
 * Asteroid::Object, so integration is a single indexed loop instead of a
 * registry query. The math and the order relative to the collision pass are
 * kept identical to the ECS branch for direct comparison.
 */

#pragma once

#include "Asteroid.h"
#include <pain.h>
#include <vector>

namespace obj::Systems
{

/** @brief Kinematic integration for plain-object asteroids. */
struct Kinematics {
  /**
   * @brief Integrates every asteroid position with its velocity.
   *
   * @param asteroids Asteroids to move.
   * @param deltaTime Frame delta time.
   */
  static void onUpdate(std::vector<Asteroid::Object> &asteroids,
                       pain::DeltaTime deltaTime);
};

} // namespace obj::Systems
