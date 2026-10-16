/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file NativeScriptSys.h
 * @brief Sparse set alternative of the archetype native script system.
 *
 * This is the same system as Scripting/Native/NativeScriptSys.h, ported from
 * the archetype registry to the sparse set registry. The only structural change
 * is how entities are enumerated:
 *
 *  - ArcheRegistry::query() returns a std::vector<reg::ChunkView<...>>, one
 *    view per archetype, and the caller indexes chunk.arrays by loop index.
 *  - SparseRegistry::view() returns a single reg::View<...> over one dense
 *    array per component, and the caller hands a lambda to forEach().
 *
 * Everything else - which callbacks are invoked, in which order, and the guards
 * around them - is deliberately identical, so the two implementations can be
 * compared directly. Note that the update, event and render bodies share the
 * names _implOnUpdate / _implOnEvent / _implOnRender with the other systems in
 * this folder; they are distinct overloads and are kept identical to the
 * archetype version on purpose.
 *
 * @note The class name, namespace, Tags and callbacks are kept identical to the
 * archetype version, so aliasesSystems.h needs no change when this header is
 * swapped in. Because both spellings define the same symbols, the archetype and
 * the sparse set variant must never be included in the same translation unit.
 *
 * @note As with any system, the callback execution is only enabled because the
 * system inherits the corresponding interface:
 *  - IOnUpdate  → enables onUpdate() callbacks.
 *  - IOnEvent   → enables onEvent() callbacks.
 *  - IOnRender  → enables onRender() callbacks.
 */

#pragma once
#include "Assets/DeltaTime.h"
#include "ECS/Components/ComponentManager.h"
#include "SparseSetAlternatives/Systems.h"
#include <pain.h>

namespace tcc
{

namespace Systems
{

namespace detail
{
void _implOnUpdate(pain::DeltaTime deltaTime,
                   reg::View<pain::NativeScriptComponent> viewScripts);
void _implOnEvent(const SDL_Event &e,
                  reg::View<pain::NativeScriptComponent> viewScripts);
void _implOnRender(pain::RenderApi &renderAPI, pain::DeltaTime currentTime,
                   reg::View<pain::NativeScriptComponent> viewScripts);
} // namespace detail

/**
 * @brief Executes native C++ script callbacks attached to entities.
 *
 * NativeScript is an ECS system that processes entities containing
 * NativeScriptComponent and dispatches registered callbacks for:
 *  - Per-frame updates
 *  - SDL events
 *  - Rendering
 *
 * The system queries all NativeScriptComponent instances and invokes the
 * bound function pointers if a script instance exists and the callback is
 * registered.
 *
 * @note This system declares a Tags list to enforce at compile time that
 * NativeScriptComponent is registered in the component manager before the
 * system can be added to a scene.
 *
 * @see System
 * @see IOnUpdate
 * @see IOnEvent
 * @see IOnRender
 * @see NativeScriptComponent
 */
template <reg::CompileTimeBitMaskType CM>
struct NativeScript : public System<CM>, IOnUpdate, IOnEvent, IOnRender {
  /**
   * @brief Component tags required by this system.
   *
   * The system operates exclusively on entities that contain
   * NativeScriptComponent. This tag list is validated at compile time when
   * inserting the system into a scene.
   */
  using Tags = TypeList<pain::NativeScriptComponent>;

  /** @brief Inherit base System constructors. */
  using System<CM>::System;

  /**
   * @brief Executes native script update callbacks for all script components.
   *
   * Called once per frame during the update phase.
   *
   * @param deltaTime Frame delta time.
   *
   * @note This method will only be called if the system inherits from
   * IOnUpdate.
   */
  void onUpdate(pain::DeltaTime deltaTime) override
  {
    detail::_implOnUpdate(deltaTime,
                          this->template view<pain::NativeScriptComponent>());
  }

  /**
   * @brief Dispatches SDL events to native script callbacks.
   *
   * Invoked whenever an SDL event is propagated through the engine.
   *
   * @param e Incoming SDL event.
   *
   * @note This method will only be called if the system inherits from
   * IOnEvent.
   */
  void onEvent(const SDL_Event &e) override
  {
    detail::_implOnEvent(e, this->template view<pain::NativeScriptComponent>());
  }

  /**
   * @brief Executes native script rendering callbacks.
   *
   * Called during the render phase after renderer state has been resolved.
   *
   * @param renderer Active 2D renderer.
   * @param currentTime Current frame time or delta time reference.
   *
   * @note This method will only be called if the system inherits from
   * IOnRender.
   */
  void onRender(pain::RenderApi &renderAPI,
                pain::DeltaTime currentTime) override
  {
    detail::_implOnRender(renderAPI, currentTime,
                          this->template view<pain::NativeScriptComponent>());
  }
  pain::RenderPass getRenderPass() const override
  {
    return pain::RenderPass::Script;
  }
};

} // namespace Systems
} // namespace tcc
