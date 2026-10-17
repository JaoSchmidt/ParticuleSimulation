#pragma once
#if 0
#include "SparseSetAlternatives/KinematicsSys.h"
#include "SparseSetAlternatives/NaiveCollisionSys.h"
#include "SparseSetAlternatives/NativeScriptSys.h"
#include "SparseSetAlternatives/Render2dSys.h"
#include "SparseSetAlternatives/WorldScene.h"
#endif
#include <pain.h>

namespace Sys
{
using Kinematics = pain::Systems::Kinematics<pain::WorldComponents>;
using NativeScript = pain::Systems::NativeScript<pain::WorldComponents>;
using Render2d = pain::Systems::Render2d<pain::WorldComponents>;
using NaiveCollision = pain::Systems::NaiveCollisionSys<pain::WorldComponents>;

#if 0
using AltKinematics = tcc::Systems::Kinematics<pain::WorldComponents>;
using AltNativeScript = tcc::Systems::NativeScript<pain::WorldComponents>;
using AltRender2d = tcc::Systems::Render2d<pain::WorldComponents>;
using AltNaiveCollision =
    tcc::Systems::NaiveCollisionSys<pain::WorldComponents>;

#endif
/**
 * @brief Creates the world scene and registers the systems it needs.
 */
pain::Scene &declareArchetypeSystems(pain::Application *app)
{
  pain::Scene &scene = app->getWorldScene();
  scene.createComponents(scene.getEntity(), cmp::Script{});
  scene.addSystem<NaiveCollision>();
  scene.addSystem<Render2d>();
  scene.addSystem<NativeScript>();
  scene.addSystem<Kinematics>();
  return scene;
}

#if 0
/**
 * @brief Creates the world scene and registers the systems it needs.
 */
tcc::SparseSetScene &declareSparseSetSystems(tcc::Application *app)
{
  tcc::SparseSetScene &scene = app->getWorldScene();
  scene.createComponents(scene.getEntity(), cmp::Script{});
  scene.addSystem<AltNaiveCollision>();
  scene.addSystem<AltRender2d>();
  scene.addSystem<AltNativeScript>();
  scene.addSystem<AltKinematics>();

  //
}
#endif
} // namespace Sys
