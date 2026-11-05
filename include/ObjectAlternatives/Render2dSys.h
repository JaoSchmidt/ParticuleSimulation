/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file Render2dSys.h
 * @brief Plain-object equivalent of the ECS 2D rendering system.
 *
 * Each object submits its own draw call through obj::IObject, instead of a
 * system querying the render archetypes.
 */

#pragma once

#include "ObjectAlternatives/IObject.h"
#include <pain.h>

namespace obj::Systems
{

/** @brief Submits plain objects to the 2D renderer. */
struct Render2d {
  /**
   * @brief Emits the draw calls for every object.
   *
   * @param renderApi Active render API.
   * @param objects   All objects.
   */
  static void onRender(pain::RenderApi &renderApi, const ObjectList &objects);
};

} // namespace obj::Systems
