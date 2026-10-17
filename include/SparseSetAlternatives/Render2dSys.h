/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file Render2dSys.h
 * @brief Sparse set alternative of the archetype 2D rendering system.
 *
 * This is the same system as CoreRender/Render2dSys.h, ported from the
 * archetype registry to the sparse set registry. The structural changes are
 * confined to how entities are enumerated:
 *
 *  - ArcheRegistry::queryConst() returns a
 *    std::vector<reg::ChunkViewConst<...>>, one view per archetype, and the
 *    caller indexes chunk.arrays by loop index.
 *  - SparseRegistry::viewConst() returns a single reg::View<...> over one dense
 *    array per component, and the caller hands a lambda to forEach().
 *
 * Everything else - which component combinations are drawn, in which order,
 * and with which renderer calls - is deliberately identical, so the two
 * implementations can be compared directly.
 *
 * @note The sparse registry exposes no const view type: viewConst() returns the
 * same reg::View<Components...> (yielding Components&) as view(), because a
 * SparseSet stores one std::vector<C> and cannot project a std::vector<const
 * C>. The call sites still go through viewConst() to keep the read-only intent,
 * and the lambdas in the implementation take the components by const reference,
 * but const-correctness is enforced by convention here rather than by the type
 * system, unlike the archetype version's ChunkViewConst.
 *
 * @note This variant lives in namespace tcc and refers to every engine type by
 * its fully qualified pain:: name, so it can coexist with the archetype system
 * inside the same binary.
 *
 * @note As with any system, the render callback is enabled only because the
 * system inherits the corresponding interface:
 *  - IOnRender → enables onRender() callbacks.
 */

#pragma once

#include "Assets/DeltaTime.h"
#include "Core.h"
#include "CoreRender/MeshComponent.h"
#include "ECS/Components/ComponentManager.h"
#include "Physics/Movement3dComponent.h"
#include "SparseSetAlternatives/Systems.h"
#include <pain.h>

namespace tcc::Systems
{

namespace detail
{
/**
 * @brief Body of the render pass, expressed over sparse set views.
 *
 * The archetype variant receives one ChunkViewConst per archetype; here each
 * parameter is the single View covering every entity owning those components,
 * and the work moves into the lambda passed to View::forEach.
 */
void _implOnRender(
    pain::RenderApi &renderer, pain::DeltaTime currentTime,
    reg::View<pain::Transform2dComponent, pain::SpriteComponent,
              pain::RotationComponent, pain::MaterialComponent>
        viewRotation,
    reg::View<pain::Transform2dComponent, pain::SpriteComponent,
              pain::MaterialComponent>
        viewTexture,
    reg::View<pain::Transform2dComponent, pain::SpriteComponent,
              pain::ColorIndexComponent, pain::MaterialComponent>
        viewColored,
    reg::View<pain::Transform2dComponent, pain::TextComponent> viewText);
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
struct Render2d : public System<CM>, IOnRender {
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
  using Tags = TypeList<pain::Transform2dComponent, //
                        pain::SpriteComponent,      //
                        pain::RotationComponent,    //
                        pain::Transform3dComponent, //
                        pain::MeshComponent,        //
                        pain::MaterialComponent,    //
                        pain::ColorIndexComponent>;

  /** @brief Inherit base System constructors. */
  using System<CM>::System;

  /**
   * @brief Submits renderable ECS components to the renderer.
   *
   * Iterates over relevant ECS component views and translates component data
   * into draw commands for the renderer. This function is typically executed
   * once per frame during the render phase of the engine pipeline.
   *
   * @param renderer     Active renderer instance used for issuing draw calls.
   * @param currentTime  Current frame time.
   *
   * @note This method is invoked only because the system inherits from
   * IOnRender.
   */
  void onRender(pain::RenderApi &renderer, pain::DeltaTime currentTime) override
  {
    detail::_implOnRender(
        renderer, currentTime,
        this->template viewConst<pain::Transform2dComponent,
                                 pain::SpriteComponent, pain::RotationComponent,
                                 pain::MaterialComponent>(
            exclude<pain::ColorIndexComponent>),
        this->template viewConst<pain::Transform2dComponent,
                                 pain::SpriteComponent,
                                 pain::MaterialComponent>(
            exclude<pain::RotationComponent, pain::ColorIndexComponent>),
        this->template viewConst<
            pain::Transform2dComponent, pain::SpriteComponent,
            pain::ColorIndexComponent, pain::MaterialComponent>(
            exclude<pain::RotationComponent>),
        this->template viewConst<pain::Transform2dComponent,
                                 pain::TextComponent>(
            exclude<pain::RotationComponent>));
  }
  pain::RenderPass getRenderPass() const override
  {
    return pain::RenderPass::Dim2d;
  }
};

} // namespace tcc::Systems
