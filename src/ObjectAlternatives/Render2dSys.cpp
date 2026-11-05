/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ObjectAlternatives/Render2dSys.h"

#include "CoreRender/Renderer/RenderApi.h"
#include "Debugging/Profiling.h"

namespace obj::Systems
{

void Render2d::onRender(pain::RenderApi &renderApi, const ObjectList &objects)
{
  PROFILE_SCOPE("Scene::renderSystems - Render2d");
  pain::Renderer2d &renderer2d = renderApi.m_renderer2d;
  for (const auto &object : objects)
    object->render(renderer2d);
  // The Render2d system is still registered in this mode, so it keeps
  // rendering the ECS leftovers (camera grid, mouse pointer) and flushing the
  // RenderContext. Nothing to replicate here.
}

} // namespace obj::Systems
