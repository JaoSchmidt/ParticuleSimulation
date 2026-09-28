#pragma once
#include <pain.h>

namespace Sys
{
using LuaScript = pain::Systems::LuaScript<pain::WorldComponents>;
using Kinematics = pain::Systems::Kinematics<pain::WorldComponents>;
using LuaScheduler = pain::Systems::LuaSchedulerSys<pain::WorldComponents>;
using NativeScript = pain::Systems::NativeScript<pain::WorldComponents>;
using Render2d = pain::Systems::Render2d<pain::WorldComponents>;
using NaiveCollision = pain::Systems::NaiveCollisionSys<pain::WorldComponents>;

/**
 * @brief Creates the world scene and registers the systems it needs.
 *
 * Both branches register the same six systems in the same order, so the two
 * runs differ only in which implementation of Render2d is used, not in how
 * many systems run per frame. The ECS asteroids are stepped by Kinematics and
 * drawn from their sprite components; the object asteroids step and draw
 * themselves, and only need the script system for their callbacks plus a
 * render system to flush the RenderContext.
 */
pain::Scene &declareSystems(pain::Application *app)
{
  pain::Scene &scene = app->getWorldScene();
  scene.createComponents(scene.getEntity(), cmp::Script{});
  scene.addSystem<NaiveCollision>();
  scene.addSystem<Render2d>();
  scene.addSystem<NativeScript>();
  scene.addSystem<LuaScript>();
  scene.addSystem<Kinematics>();
  scene.addSystem<LuaScheduler>();
  return scene;
}
} // namespace Sys
