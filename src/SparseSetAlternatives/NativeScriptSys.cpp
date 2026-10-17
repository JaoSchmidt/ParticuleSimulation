/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "SparseSetAlternatives/NativeScriptSys.h"
#include "CoreRender/Renderer/RenderApi.h"
#include "Debugging/Profiling.h"
#include "ECS/Components/NativeScript.h"

namespace tcc::Systems::detail
{

void _implOnUpdate(pain::DeltaTime deltaTime,
                   reg::View<pain::NativeScriptComponent> viewScripts)
{
  PROFILE_SCOPE("Scene::updateSystems - native scripts");

  viewScripts.forEach([&](pain::NativeScriptComponent &nsc) {
    if (nsc.instance && nsc.onUpdateFunction) {
      nsc.onUpdateFunction(nsc.instance.get(), deltaTime);
    }
  });
}

void _implOnEvent(const SDL_Event &e,
                  reg::View<pain::NativeScriptComponent> viewScripts)
{
  viewScripts.forEach([&](pain::NativeScriptComponent &nsc) {
    if (nsc.instance && nsc.onEventFunction) {
      nsc.onEventFunction(nsc.instance.get(), e);
    }
  });
}

void _implOnRender(pain::RenderApi &renderAPI, pain::DeltaTime currentTime,
                   reg::View<pain::NativeScriptComponent> viewScripts)
{
  UNUSED(renderAPI);
  PROFILE_SCOPE("Scene::renderSystems - NativeScripts");

  viewScripts.forEach([&](pain::NativeScriptComponent &nsc) {
    if (nsc.instance && nsc.onRenderFunction) {
      nsc.onRenderFunction(nsc.instance.get(), renderAPI, currentTime);
    }
  });
}

} // namespace tcc::Systems::detail
