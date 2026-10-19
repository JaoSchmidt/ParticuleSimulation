/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// RenderPipeline.h
#pragma once

#include "AlternativeScene.h"
#include "CoreRender/Buffers/FrameBuffer.h"
#include "CoreRender/Renderer/RenderApi.h"
#include "Debugging/Profiling.h"
#include "ECS/Scene.h"
#include "Misc/Events.h"

namespace tcc
{
namespace detail
{
void _implOnWindowResized(
    const SDL_Event &event, pain::RenderApi &renderAPI,
    pain::FrameBuffer &framebuffer,
    reg::View<pain::OrthoCameraComponent> viewOrthoCameras,
    reg::View<pain::PerspCameraComponent> viewPerspCameras);
void _implResizeFBViewport(
    const pain::ImGuiViewportChangeEvent &event,
    reg::View<pain::OrthoCameraComponent> viewOrthoCameras,
    pain::FrameBuffer &frameBuffer);
void _implResizeFBViewport(
    const pain::ImGuiViewportChangeEvent &event,
    reg::View<pain::PerspCameraComponent> viewPerspCameras,
    pain::FrameBuffer &frameBuffer);
void _implChangeActiveCameraEvent(
    reg::View<pain::OrthoCameraComponent> viewOrthoCameras,
    pain::OrthoCameraComponent &cam, pain::RenderApi &renderAPI);
std::optional<
    std::pair<const std::reference_wrapper<pain::OrthoCameraComponent>,
              const std::reference_wrapper<pain::Transform2dComponent>>>
_implRetrieve2dCamera(
    reg::View<pain::OrthoCameraComponent, pain::Transform2dComponent>
        viewCameras2d);
std::optional<
    std::pair<const std::reference_wrapper<pain::PerspCameraComponent>,
              const std::reference_wrapper<pain::Transform3dComponent>>>
_implRetrieve3dCamera(
    reg::View<pain::PerspCameraComponent, pain::Transform3dComponent>
        viewCameras3d);
} // namespace detail
/**
 * @class RenderPipeline
 * @brief Manages the high-level rendering flow of the engine.
 *
 * The RenderPipeline owns the main framebuffer and coordinates rendering
 * of the World scene and UI scene. It also reacts to window and viewport
 * resize events and updates camera projections accordingly.
 */
class RenderPipeline
{
public:
  /**
   * @brief Creates a render pipeline using the *default* framebuffer
   * configuration.
   *
   * This overload creates a framebuffer configured as a swap-chain target.
   *
   * @param eventDispatcher Event dispatcher used for viewport subscriptions.
   * @return Constructed RenderPipeline instance.
   */
  static RenderPipeline create(reg::EventDispatcher &eventDispatcher);

  /**
   * @brief Creates a render pipeline using a custom framebuffer configuration.
   *
   * @param info Framebuffer creation parameters.
   * @param eventDispatcher Event dispatcher used for viewport subscriptions.
   * @return Constructed RenderPipeline instance.
   */
  static RenderPipeline create(const pain::FrameBufferCreationInfo &info,
                               reg::EventDispatcher &eventDispatcher);

  /**
   * @brief Handles window resize events and updates cameras and viewport.
   *
   * This function propagates resize events to all active orthographic cameras
   * in the provided scene and resizes the framebuffer when required.
   *
   * @param event SDL window event.
   * @param renderer Active renderer instance.
   * @param scene Scene containing camera components.
   */
  template <reg::CompileTimeBitMaskType WC>
  inline void onWindowResized(const SDL_Event &event,
                              pain::RenderApi &renderApi,
                              tcc::AlternativeScene<WC> &scene)
  {
    detail::_implOnWindowResized(
        event, renderApi, m_frameBuffer,
        scene.template view<pain::OrthoCameraComponent>(),
        scene.template view<pain::PerspCameraComponent>());
  }

  /**
   * @brief Executes a full render pass for the current frame.
   *
   * The pipeline performs:
   * - Framebuffer binding and clearing
   * - World scene rendering
   * - Framebuffer unbinding
   * - UI scene rendering
   *
   * @param renderer Active renderer instance.
   * @param currentTime Current frame delta time.
   * @param worldScene Scene containing world entities and systems.
   * @param uiScene Scene containing UI entities and systems.
   */
  template <reg::CompileTimeBitMaskType WC, reg::CompileTimeBitMaskType UIC>
  void pipeline(pain::RenderApi &renderAPI, bool isRenderingEnabled,
                pain::DeltaTime currentTime,
                tcc::AlternativeScene<WC> &worldScene,
                tcc::AlternativeScene<UIC> *uiScene)
  {
    PROFILE_FUNCTION();
    m_frameBuffer.bind();
    pain::backend::clear();
    // TODO: putting isRenderingEnabled here means there is no need for passing
    // it through every single onRender function like we are doing now. Removing
    // should be a task eventually
    if (!isRenderingEnabled) {
      auto wrap2d = detail::_implRetrieve2dCamera(
          worldScene.template view<pain::OrthoCameraComponent,
                                   pain::Transform2dComponent>());
      auto wrap3d = detail::_implRetrieve3dCamera(
          worldScene.template view<pain::PerspCameraComponent,
                                   pain::Transform3dComponent>());

      // Scripts don't actually use renderAPI, they fill the render context
      // worldScene.renderSystems(RenderPass::Script, renderAPI, currentTime);

      P_ASSERT_W(wrap3d || wrap2d, "No active default camera");
      if (wrap3d) {
        pain::backend::enable3d();
        renderAPI.m_renderer3d.beginScene(currentTime, wrap3d->first,
                                          wrap3d->second);
        worldScene.renderSystems(pain::RenderPass::Dim3d, renderAPI,
                                 currentTime);
        renderAPI.m_renderer3d.endScene();
      }
      if (wrap2d) {
        pain::backend::disable3d();
        renderAPI.m_renderer2d.beginScene();
        // HACK: Temporary using render api directly inside scripts
        worldScene.renderSystems(pain::RenderPass::Dim2d, renderAPI,
                                 currentTime);
        worldScene.renderSystems(pain::RenderPass::Script, renderAPI,
                                 currentTime);
        renderAPI.m_renderer2d.endScene(currentTime, wrap2d->first,
                                        wrap2d->second);
      }
      renderAPI.m_renderContext.clear();
    }
    m_frameBuffer.unbind();
    if (uiScene != nullptr)
      uiScene->renderSystems(pain::RenderPass::UI, renderAPI, currentTime);
  }

  /**
   * @brief Subscribes to viewport change events for camera updates.
   *
   * When an ImGui viewport change event occurs, all orthographic cameras
   * in the scene will update their projection and framebuffer size.
   *
   * @param scene Scene containing camera components.
   */
  template <reg::CompileTimeBitMaskType WC>
  void subscribeToEvents(tcc::AlternativeScene<WC> &scene,
                         pain::RenderApi &renderAPI)
  {
    m_eventDispatcher.subscribe<pain::ImGuiViewportChangeEvent>(
        [&](const pain::ImGuiViewportChangeEvent &e) {
          detail::_implResizeFBViewport(
              e, scene.template view<pain::OrthoCameraComponent>(),
              m_frameBuffer);
          detail::_implResizeFBViewport(
              e, scene.template view<pain::PerspCameraComponent>(),
              m_frameBuffer);
        });
    m_eventDispatcher.subscribe<pain::ChangeActiveCameraEvent>(
        [&](const pain::ChangeActiveCameraEvent &e) {
          auto &cam =
              scene.template getComponent<pain::OrthoCameraComponent>(e.cam);
          if (cam.m_active) {
            auto viewOrthoCameras =
                scene.template view<pain::OrthoCameraComponent>();
            detail::_implChangeActiveCameraEvent(viewOrthoCameras, cam,
                                                 renderAPI);
          }
        });
  };

  /** @brief Framebuffer owned by the render pipeline. */
  pain::FrameBuffer m_frameBuffer;

  /**  @brief Event dispatcher used for resize and viewport events. */
  reg::EventDispatcher &m_eventDispatcher;

private:
  /**
   * @brief Constructs a render pipeline from an existing framebuffer.
   *
   * Intended to be used internally by the factory methods.
   *
   * @param frameBuffer Framebuffer instance.
   * @param eventDispatcher Event dispatcher reference.
   */
  explicit RenderPipeline(pain::FrameBuffer frameBuffer,
                          reg::EventDispatcher &eventDispatcher);
};

} // namespace tcc
