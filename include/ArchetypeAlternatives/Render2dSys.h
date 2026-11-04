/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file RenderSys.h
 * @brief ECS rendering system responsible for submitting visible 2D primitives
 *        and sprites to the renderer.
 *
 * Render is the primary rendering system for the 2D layer of the engine. It
 * queries renderable components from the ECS and submits draw commands to the
 * active Renderer2d instance. Rendering is organized by component signatures
 * and supports multiple renderable types such as:
 *
 *  - Sprites with or without rotation.
 *  - Shape-based primitives (quads, circles, triangles).
 *  - Particle systems.
 *
 * The system itself performs no rendering logic or batching; it acts purely as
 * a bridge between ECS component data and the renderer API.
 *
 * @note As with any system in the engine, system headers follow the naming
 * convention and end with the suffix "Sys.h".
 *
 * @note As with any system, the render callback is enabled only because the
 * system inherits the corresponding interface:
 *  - IOnRender → enables onRender() callbacks.
 *
 * If the interface is removed, the engine will no longer invoke the render pass
 * for this system.
 */

#pragma once

#include "Assets/DeltaTime.h"
#include "Core.h"
#include "CoreRender/MeshComponent.h"
#include "ECS/Components/ComponentManager.h"
#include "ECS/Systems.h"
#include "Physics/Movement3dComponent.h"

namespace pain
{
struct Renderer2d;
struct Transform2dComponent;
struct RotationComponent;
struct SpriteComponent;
struct MaterialComponent;
} // namespace pain

namespace tcc::Systems
{

namespace detail
{
void _implOnRender(
    pain::RenderApi &renderer, pain::DeltaTime currentTime,
    const std::vector<reg::ChunkViewConst<
        const pain::Transform2dComponent, const pain::SpriteComponent,
        const pain::RotationComponent, const pain::MaterialComponent>>
        &chunksRotation,
    const std::vector<reg::ChunkViewConst<
        const pain::Transform2dComponent, const pain::SpriteComponent,
        const pain::MaterialComponent>> &chunksTexture);
} // namespace detail

/**
 * @brief ECS system responsible for submitting all visible renderable
 * components to the 2D renderer.
 *
 * The Render System iterates over all sprites, meshes and materials components
 * to register inside the Renderer. The system does not perform visibility
 * culling, sorting, or batching; those responsibilities are delegated to the
 * renderer and higher-level scene management systems.
 *
 * Entities processed by this system must provide at minimum the components
 * listed in the Tags type. Additional component combinations are queried
 * internally to support optional features such as rotation and different
 * primitive types.
 *
 * @note As with any system, the render callback is only active because this
 * system inherits from IOnRender.
 *
 * @see Renderer2d
 * @see Transform2dComponent
 * @see SpriteComponent
 * @see RotationComponent
 * @see System
 * @see IOnRender
 */
template <reg::CompileTimeBitMaskType CM>
struct AltRender2d : public pain::System<CM>, pain::IOnRender {
  /**
   * @brief Component signature required by this system.
   *
   * Declares that this system operates on entities containing:
   *  - Transform2dComponent
   *  - SpriteComponent
   *  - RotationComponent
   *
   * Additional queries may selectively include or exclude components at
   * runtime.
   */
  using Tags = pain::TypeList<pain::Transform2dComponent, //
                              pain::SpriteComponent,      //
                              pain::RotationComponent,    //
                              pain::MeshComponent,        //
                              pain::MaterialComponent     //
                              >;

  /** @brief Inherit base System constructors. */
  using pain::System<CM>::System;

  /**
   * @brief Submits renderable ECS components to the renderer.
   *
   * Iterates over relevant ECS component chunks and translates component data
   * into draw commands for the renderer. This function is typically executed
   * once per frame during the render phase of the engine pipeline.
   *
   * @param renderer     Active renderer instance used for issuing draw calls.
   * @param currentTime  Current frame time.
   *
   * @note This method is invoked only because the system inherits from
   * IOnSystemRender.
   */
  void onRender(pain::RenderApi &renderer, pain::DeltaTime currentTime) override
  {
    detail::_implOnRender(
        renderer, currentTime,
        this->template queryConst<
            pain::Transform2dComponent, pain::SpriteComponent,
            pain::RotationComponent, pain::MaterialComponent>(),
        this->template queryConst<pain::Transform2dComponent,
                                  pain::SpriteComponent,
                                  pain::MaterialComponent>(
            exclude<pain::RotationComponent>));
  }
  pain::RenderPass getRenderPass() const override
  {
    return pain::RenderPass::Dim2d;
  }
};

} // namespace tcc::Systems
