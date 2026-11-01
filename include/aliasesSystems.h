#pragma once
#include "ArchetypeAlternatives/CollisionNaiveSys.h"
#include "SparseSetAlternatives/Application.h"
#include "SparseSetAlternatives/KinematicsSys.h"
#include "SparseSetAlternatives/CollisionNaiveSys.h"
#include "SparseSetAlternatives/NativeScriptSys.h"
#include "SparseSetAlternatives/Render2dSys.h"
#include "SparseSetAlternatives/WorldScene.h"
#include <pain.h>

namespace Sys
{
using Kinematics = pain::Systems::Kinematics<WorldComponents>;
using NativeScript = pain::Systems::NativeScript<WorldComponents>;
using Render2d = pain::Systems::Render2d<WorldComponents>;
using NaiveCollision = pain::Systems::NaiveCollisionSys<WorldComponents>;
using ArchAltNaiveCollision =
    pain::Systems::ArchAltNaiveCollisionSys<WorldComponents>;

using AltKinematics = tcc::Systems::Kinematics<WorldComponents>;
using AltNativeScript = tcc::Systems::NativeScript<WorldComponents>;
using AltRender2d = tcc::Systems::Render2d<WorldComponents>;
using AltNaiveCollision = tcc::Systems::NaiveCollisionSys<WorldComponents>;

/**
 * @brief Creates the world scene and registers the systems it needs.
 */
pain::Scene &declareArchetypeSystems(pain::Application *app)
{
  pain::Scene &scene = app->getWorldScene();
  scene.createComponents(scene.getEntity(), cmp::Script{});
  scene.addSystem<ArchAltNaiveCollision>();
  scene.addSystem<Render2d>();
  scene.addSystem<NativeScript>();
  scene.addSystem<Kinematics>();
  return scene;
}

/**
 * @brief Creates the world scene and registers the systems it needs.
 */
tcc::SparseSetScene &declareSparseSetSystems(tcc::Application *app)
{
  tcc::SparseSetScene &scene = app->getWorldScene();
  scene.createComponents(scene.getEntity(), tcc::NativeScriptComponent{});
  scene.addSystem<AltNaiveCollision>();
  scene.addSystem<AltRender2d>();
  scene.addSystem<AltNativeScript>();
  scene.addSystem<AltKinematics>();
  return scene;
}
} // namespace Sys
