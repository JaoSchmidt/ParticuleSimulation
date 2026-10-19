
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "SparseSetAlternatives/RenderPipeline.h"
#include "CoreRender/Buffers/FrameBuffer.h"
#include "CoreRender/CameraComponent.h"
#include "CoreRender/Renderer/RenderApi.h"
#include "Misc/Events.h"
#include "Physics/Movement3dComponent.h"
#include "Physics/MovementComponent.h"
#include "platform/ContextBackend.h"

#include <cstdlib>

// NOTE: technically this pipeline is incomplete because it doesn't take into
// account multiple screens

namespace tcc
{
namespace
{
constexpr std::array<glm::vec4, 3> s_colorOptions = {
    glm::vec4(0.0F, 0.0F, 0.0F, 1.0F), // Pure Black
    glm::vec4(0.2F, 0.2F, 0.2F, 1.0F), // Dark Grey
    glm::vec4(1.0F, 0.2F, 0.9F, 1.0F)  // Strong Pink
};
constexpr glm::vec4 s_clearColor = s_colorOptions[1];
} // namespace

RenderPipeline::RenderPipeline(pain::FrameBuffer frameBuffer,
                               reg::EventDispatcher &eventDispatcher)
    : m_frameBuffer(std::move(frameBuffer)),
      m_eventDispatcher(eventDispatcher) {};

RenderPipeline RenderPipeline::create(reg::EventDispatcher &eventDispatcher)

{
  return RenderPipeline::create(
      pain::FrameBufferCreationInfo{.swapChainTarget = true}, eventDispatcher);
}

RenderPipeline RenderPipeline::create(const pain::FrameBufferCreationInfo &info,
                                      reg::EventDispatcher &eventDispatcher)
{
  std::optional<pain::FrameBuffer> fb = pain::FrameBuffer::create(info);
  if (!fb.has_value()) {
    PLOG_E("FrameBuffer wasn't able to be created!");
    std::exit(1);
  }

  pain::backend::setClearColor(s_clearColor);
  return RenderPipeline{std::move(*fb), eventDispatcher};
}

template <typename Camera>
  requires std::same_as<Camera, pain::PerspCameraComponent> ||
           std::same_as<Camera, pain::OrthoCameraComponent>
void resizeCamera(const SDL_Event &event, Camera &c, pain::FrameBuffer &fb,
                  pain::RenderApi &renderAPI)
{
  if (fb.getSpecification().swapChainTarget) {
    renderAPI.setViewPort(0, 0, event.window.data1, event.window.data2);
    c.setProjection(event.window.data1, event.window.data2);
  } else {
    // TODO: for minimaps/split screens bc (0,0,w,h) viewport won't work
    c.setProjection(fb.getWidthi(), fb.getHeighti());
    fb.resizeFrameBuffer(fb.getWidthi(), fb.getHeighti());
  }
}

namespace detail
{

void _implResizeFBViewport(
    const pain::ImGuiViewportChangeEvent &event,
    reg::View<pain::OrthoCameraComponent> viewOrthoCameras,
    pain::FrameBuffer &frameBuffer)
{
  const int newx = static_cast<int>(event.newSize.x);
  const int newy = static_cast<int>(event.newSize.y);
  frameBuffer.resizeFrameBuffer(newx, newy);
  viewOrthoCameras.forEach([&](pain::OrthoCameraComponent &camera) {
    camera.setProjection(newx, newy);
  });
}

void _implResizeFBViewport(
    const pain::ImGuiViewportChangeEvent &event,
    reg::View<pain::PerspCameraComponent> viewPerspCameras,
    pain::FrameBuffer &frameBuffer)
{
  const int newx = static_cast<int>(event.newSize.x);
  const int newy = static_cast<int>(event.newSize.y);
  frameBuffer.resizeFrameBuffer(newx, newy);
  viewPerspCameras.forEach([&](pain::PerspCameraComponent &camera) {
    camera.setProjection(newx, newy);
  });
}

void _implChangeActiveCameraEvent(
    reg::View<pain::OrthoCameraComponent> viewOrthoCameras,
    pain::OrthoCameraComponent &cam, pain::RenderApi &renderAPI)
{
  viewOrthoCameras.forEach(
      [&](pain::OrthoCameraComponent &camera) { camera.m_active = false; });
  renderAPI.setViewPort(cam.m_screenPosition.x, cam.m_screenPosition.y,
                        cam.m_resolution.x, cam.m_resolution.y);
}

void _implOnWindowResized(
    const SDL_Event &event, pain::RenderApi &renderer,
    pain::FrameBuffer &frameBuffer,
    reg::View<pain::OrthoCameraComponent> viewOrthoCameras,
    reg::View<pain::PerspCameraComponent> viewPerspCameras)
{
  viewOrthoCameras.forEach([&](pain::OrthoCameraComponent &camera) {
    if (event.type == SDL_WINDOWEVENT) {
      if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        resizeCamera(event, camera, frameBuffer, renderer);
      }
    }
  });
  viewPerspCameras.forEach([&](pain::PerspCameraComponent &camera) {
    if (event.type == SDL_WINDOWEVENT) {
      if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
        resizeCamera(event, camera, frameBuffer, renderer);
      }
    }
  });
}

std::optional<
    std::pair<const std::reference_wrapper<pain::OrthoCameraComponent>,
              const std::reference_wrapper<pain::Transform2dComponent>>>
_implRetrieve2dCamera(
    reg::View<pain::OrthoCameraComponent, pain::Transform2dComponent>
        viewCameras2d)
{
  std::optional<std::pair<std::reference_wrapper<pain::OrthoCameraComponent>,
                          std::reference_wrapper<pain::Transform2dComponent>>>
      activeCamera;
  viewCameras2d.forEach([&](pain::OrthoCameraComponent &camera,
                            pain::Transform2dComponent &transform) {
    if (activeCamera)
      return;
    if (camera.m_active) {
      activeCamera = std::pair(std::ref(camera), std::ref(transform));
    }
  });
  return activeCamera;
}
std::optional<
    std::pair<const std::reference_wrapper<pain::PerspCameraComponent>,
              const std::reference_wrapper<pain::Transform3dComponent>>>
_implRetrieve3dCamera(
    reg::View<pain::PerspCameraComponent, pain::Transform3dComponent>
        viewCameras3d)
{
  // TODO: implement for 3D OrthoCameras?
  std::optional<std::pair<std::reference_wrapper<pain::PerspCameraComponent>,
                          std::reference_wrapper<pain::Transform3dComponent>>>
      activeCamera;
  viewCameras3d.forEach([&](pain::PerspCameraComponent &camera,
                            pain::Transform3dComponent &transform) {
    if (activeCamera)
      return;
    if (camera.m_active) {
      activeCamera = std::pair(std::ref(camera), std::ref(transform));
    }
  });
  return activeCamera;
}

} // namespace detail

} // namespace tcc
