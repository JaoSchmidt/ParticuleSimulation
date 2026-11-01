/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file CollisionNaiveSys.h
 * @brief Plain-object equivalent of the ECS naive collision system.
 *
 * The plain-object simulation keeps position, velocity and collider state
 * inside Asteroid::Object and Wall::Object, so the pair passes index those
 * vectors directly instead of walking registry chunks. Detection, reaction
 * and the order of the moving and static passes mirror the ECS branch.
 */

#pragma once

#include "Asteroid.h"
#include "Wall.h"
#include <pain.h>
#include <vector>

namespace obj::Systems
{

/** @brief Brute-force collision detection and response for plain objects. */
struct NaiveCollision {
  /**
   * @brief Resolves asteroid-vs-asteroid and asteroid-vs-wall collisions.
   *
   * @param asteroids Movable objects, updated in place.
   * @param walls     Static objects, read only.
   */
  static void onUpdate(std::vector<Asteroid::Object> &asteroids,
                       const std::vector<Wall::Object> &walls);
};

} // namespace obj::Systems
