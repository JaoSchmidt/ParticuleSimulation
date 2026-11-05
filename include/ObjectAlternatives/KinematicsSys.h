/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file KinematicsSys.h
 * @brief Plain-object equivalent of the ECS kinematics system.
 *
 * Each object integrates itself through obj::IObject, instead of a system
 * querying a registry. The math and the order relative to the collision pass
 * are kept identical to the ECS version for direct comparison.
 */

#pragma once

#include "ObjectAlternatives/IObject.h"
#include <pain.h>

namespace obj::Systems
{

/** @brief Kinematic integration for plain objects. */
struct Kinematics {
  /**
   * @brief Updates the rotation and position of every object.
   *
   * @param objects   All objects. The ones that don't move ignore the calls.
   * @param deltaTime Frame delta time.
   */
  static void onUpdate(ObjectList &objects, pain::DeltaTime deltaTime);
};

} // namespace obj::Systems
