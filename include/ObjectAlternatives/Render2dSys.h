/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file Render2dSys.h
 * @brief Plain-object equivalent of the ECS 2D rendering system.
 *
 * The plain-object simulation keeps its drawable data in Asteroid::Object and
 * Wall::Object, so this system submits those vectors directly instead of
 * querying the render archetypes. Shape dispatch, rotation handling and
 * submission order mirror the ECS branch.
 */

#pragma once

#include "Asteroid.h"
#include "Wall.h"
#include <pain.h>
#include <vector>

namespace obj::Systems
{

/** @brief Submits plain-object asteroids and walls to the 2D renderer. */
struct Render2d {
  /**
   * @brief Emits the draw calls for every plain-object entity.
   *
   * @param renderApi Active render API.
   * @param asteroids Rotated asteroid objects.
   * @param walls     Non-rotated wall objects.
   */
  static void onRender(pain::RenderApi &renderApi,
                       const std::vector<Asteroid::Object> &asteroids,
                       const std::vector<Wall::Object> &walls);
};

} // namespace obj::Systems
