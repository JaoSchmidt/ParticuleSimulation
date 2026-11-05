/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file CollisionNaiveSys.h
 * @brief Plain-object equivalent of the ECS naive collision system.
 *
 * Position, velocity and collider state are read through obj::IObject.
 * Detection, reaction and the order of the moving and static passes mirror the
 * ECS version.
 */

#pragma once

#include "ObjectAlternatives/IObject.h"
#include <pain.h>

namespace obj::Systems
{

/** @brief Brute-force collision detection and response for plain objects. */
struct NaiveCollision {
  /**
   * @brief Resolves movable-vs-movable and movable-vs-static collisions.
   *
   * @param objects         All objects, the static ones first.
   * @param staticCount     How many objects at the front of the list are static.
   * @param eventDispatcher Dispatcher used for trigger collision events.
   */
  static void onUpdate(ObjectList &objects, size_t staticCount,
                       reg::EventDispatcher &eventDispatcher);
};

} // namespace obj::Systems
