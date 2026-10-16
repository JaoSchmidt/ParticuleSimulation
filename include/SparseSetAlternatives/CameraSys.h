/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file CameraSys.h
 * @brief Sparse set alternative of the archetype camera system.
 *
 * This is the same system as Misc/CameraSys.h, ported from the archetype
 * registry to the sparse set registry. The only structural change is how
 * entities are enumerated:
 *
 *  - ArcheRegistry::query() returns a std::vector<reg::ChunkView<...>>, one
 *    view per archetype, and the caller indexes chunk.arrays by loop index.
 *  - SparseRegistry::view() returns a single reg::View<...> over one dense
 *    array per component, and the caller hands a lambda to forEach().
 *
 * Everything else - which events are handled, which components are touched, in
 * which order - is deliberately identical, so the two implementations can be
 * compared directly. This includes the archetype version's placement of the
 * SDL event checks *inside* the per-camera loop.
 *
 * @note The class name, namespace, Tags and callbacks are kept identical to the
 * archetype version, so aliasesSystems.h needs no change when this header is
 * swapped in. Because both spellings define the same symbols, the archetype and
 * the sparse set variant must never be included in the same translation unit.
 *
 * @warning This system is currently marked as work-in-progress (WIP) and should
 * not be relied upon for production usage.
 *
 * @note As with any system, the callback execution is only enabled because the
 * system inherits the corresponding interface:
 *  - IOnEvent   → enables onEvent() callbacks.
 */
#pragma once
#include "Assets/DeltaTime.h"
#include "CoreRender/CameraComponent.h"
#include "ECS/Components/ComponentManager.h"
#include "SparseSetAlternatives/Systems.h"

namespace tcc::Systems
{
/**
 * @brief Event-driven system for updating orthographic camera state.
 *
 * CameraSys reacts to SDL window events and propagates resolution and
 * projection changes into all registered OrthoCamera components. This allows
 * cameras to automatically adjust their aspect ratio and projection bounds when
 * the window is resized.
 *
 * The system does not perform any spatial updates, physics integration, or
 * rendering logic.
 *
 * @note As with any system, the callback execution is only enabled because the
 * system inherits the corresponding interface:
 *  - IOnEvent   → enables onEvent() callbacks.
 *
 * @note The Tags list is validated at compile time to ensure that all required
 * components are registered in the component manager before this system can be
 * added to a scene.
 *
 * @see OrthoCameraComponent
 * @see System
 * @see IOnEvent
 */
struct CameraSys : public System<WorldComponents>, IOnEvent {
  /**
   * @brief Component tags required by this system.
   *
   * Declares that this system operates on entities containing:
   *  - OrthoCameraComponent
   */
  using Tags = TypeList<OrthoCameraComponent>;

  /** @brief Inherit base System constructors. */
  using System::System;

  /**
   * @brief Handles incoming SDL events and updates camera state when required.
   *
   * Responds primarily to window resize events and recomputes camera projection
   * parameters such as aspect ratio and resolution for each active camera
   * component.
   *
   * @param event Incoming SDL event.
   *
   * @note This method is invoked only because the system inherits from
   * IOnEvent.
   */
  void onEvent(const SDL_Event &event) override;
};
} // namespace tcc::Systems
