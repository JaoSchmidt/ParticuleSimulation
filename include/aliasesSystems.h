#pragma once
#include "ArchetypeAlternatives/Render2dSys.h"
#include <pain.h>

namespace Sys
{
using Kinematics = pain::Systems::Kinematics<WorldComponents>;
using NativeScript = pain::Systems::NativeScript<WorldComponents>;
using Render2d = pain::Systems::Render2d<WorldComponents>;
using NaiveCollision = pain::Systems::NaiveCollisionSys<WorldComponents>;
using AltRender2d = tcc::Systems::AltRender2d<WorldComponents>;

/**
 * @brief Creates the world scene and registers the systems it needs.
 */
pain::Scene &declareArchetypeSystems(pain::Application *app)
{
  pain::Scene &scene = app->getWorldScene();
  scene.createComponents(scene.getEntity(), cmp::Script{});
  scene.addSystem<NaiveCollision>();
  scene.addSystem<AltRender2d>();
  scene.addSystem<NativeScript>();
  scene.addSystem<Kinematics>();
  return scene;
}
pain::Scene &declareObj(pain::Application *app)
{
  pain::Scene &scene = app->getWorldScene();
  scene.createComponents(scene.getEntity(), cmp::Script{});
  scene.addSystem<NativeScript>();
  return scene;
}

} // namespace Sys
