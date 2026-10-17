/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "SparseSetAlternatives/RenderPipeline.h"
#include "CoreRender/CameraComponent.h"
#include "CoreRender/Renderer/RenderApi.h"
#include "Debugging/Profiling.h"
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

template <typename Camera>
  requires std::same_as<Camera, pain::PerspCameraComponent> ||
           std::same_as<Camera, pain::OrthoCameraComponent>
void resizeFBViewport(const pain::ImGuiViewportChangeEvent &event, Camera &cc,
                      pain::FrameBuffer &frameBuffer)
{
  int newx = static_cast<int>(event.newSize.x);
  int newy = static_cast<int>(event.newSize.y);
  frameBuffer.resizeFrameBuffer(newx, newy);
  cc.setProjection(newx, newy);
}

RenderPipeline::RenderPipeline(pain::FrameBuffer frameBuffer,
                               reg::EventDispatcher &eventDispatcher)
    : m_frameBuffer(std::move(frameBuffer)),
      m_eventDispatcher(eventDispatcher) {};

void RenderPipeline::subscribeToEvents(tcc::SparseSetScene &scene,
                                       pain::RenderApi &renderAPI)
{
#if 0
  m_eventDispatcher.subscribe<pain::ImGuiViewportChangeEvent>(
      [&](const pain::ImGuiViewportChangeEvent &e) {
        auto chunks = scene.query<pain::OrthoCameraComponent>();
        for (auto &chunk : chunks) {
          auto *c = std::get<0>(chunk.arrays);

          for (size_t i = 0; i < chunk.count; ++i) {
            resizeFBViewport(e, c[i], m_frameBuffer);
          }
        }
        auto chunks2 = scene.query<pain::PerspCameraComponent>();
        for (auto &chunk : chunks2) {
          auto *c = std::get<0>(chunk.arrays);

          for (size_t i = 0; i < chunk.count; ++i) {
            resizeFBViewport(e, c[i], m_frameBuffer);
          }
        }
      });
  m_eventDispatcher.subscribe<pain::ChangeActiveCameraEvent>(
      [&](const pain::ChangeActiveCameraEvent &e) {
        auto &cam = scene.getComponent<pain::OrthoCameraComponent>(e.cam);
        if (cam.m_active) {
          auto chunks = scene.query<pain::OrthoCameraComponent>();
          for (auto &chunk : chunks) {
            auto *c = std::get<0>(chunk.arrays);
            for (size_t i = 0; i < chunk.count; ++i)
              c[i].m_active = false;
          }
          renderAPI.setViewPort(cam.m_screenPosition.x, cam.m_screenPosition.y,
                                cam.m_resolution.x, cam.m_resolution.y);
        }
      });
#endif
};

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

void RenderPipeline::onWindowResized(const SDL_Event &event,
                                     pain::RenderApi &renderer,
                                     tcc::SparseSetScene &scene)
{
#if 0
  {
    auto chunks = scene.query<pain::OrthoCameraComponent>();
    for (auto &chunk : chunks) {
      auto *c = std::get<0>(chunk.arrays);

      for (size_t i = 0; i < chunk.count; ++i) {
        if (event.type == SDL_WINDOWEVENT) {
          if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            resizeCamera(event, c[i], m_frameBuffer, renderer);
          }
        }
      }
    }
  }
  {
    auto chunks = scene.query<pain::PerspCameraComponent>();
    for (auto &chunk : chunks) {
      auto *c = std::get<0>(chunk.arrays);

      for (size_t i = 0; i < chunk.count; ++i) {
        if (event.type == SDL_WINDOWEVENT) {
          if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
            resizeCamera(event, c[i], m_frameBuffer, renderer);
          }
        }
      }
    }
  }
#endif
}

std::optional<
    std::pair<const std::reference_wrapper<pain::OrthoCameraComponent>,
              const std::reference_wrapper<pain::Transform2dComponent>>>
retrieve2dCamera(tcc::SparseSetScene &scene)
{
#if 0
  auto chunks =
      scene.query<pain::OrthoCameraComponent, pain::Transform2dComponent>();
  for (auto &chunk : chunks) {
    auto *c = std::get<0>(chunk.arrays);
    auto *t = std::get<1>(chunk.arrays);
    for (size_t i = 0; i < chunk.count; ++i) {
      if (c[i].m_active) {
        return std::pair(std::ref(c[i]), std::ref(t[i]));
      }
    }
  }
#endif
  return {};
}
std::optional<
    std::pair<const std::reference_wrapper<pain::PerspCameraComponent>,
              const std::reference_wrapper<pain::Transform3dComponent>>>
retrieve3dCamera(tcc::SparseSetScene &scene)
{
#if 0
  // TODO: implement for 3D OrthoCameras?
  auto chunks =
      scene.query<pain::PerspCameraComponent, pain::Transform3dComponent>();
  for (auto &chunk : chunks) {
    auto *c = std::get<0>(chunk.arrays);
    auto *t = std::get<1>(chunk.arrays);
    for (size_t i = 0; i < chunk.count; ++i) {
      if (c[i].m_active) {
        return std::pair(std::ref(c[i]), std::ref(t[i]));
      }
    }
  }
#endif
  return {};
}

void RenderPipeline::temp()
{
  // PLOG_I("Size = ({},{})", m_frameBuffer.getWidth(),
  // m_frameBuffer.getHeight());

  // PLOG_I("IsInside = {}",);
}

void RenderPipeline::pipeline(pain::RenderApi &renderAPI,
                              bool isRenderingEnabled,
                              pain::DeltaTime currentTime,
                              tcc::SparseSetScene &worldScene,
                              tcc::SparseSetUIScene *uiScene)
{
  PROFILE_FUNCTION();
  m_frameBuffer.bind();
  pain::backend::clear();
  // TODO: putting isRenderingEnabled here means there is no need for passing it
  // through every single onRender function like we are doing now. Removing
  // should be a task eventually
  if (!isRenderingEnabled) {
    auto wrap2d = retrieve2dCamera(worldScene);
    auto wrap3d = retrieve3dCamera(worldScene);

    // Scripts don't actually use renderAPI, they fill the render context
    // worldScene.renderSystems(RenderPass::Script, renderAPI, currentTime);

    P_ASSERT_W(wrap3d || wrap2d, "No active default camera");
    if (wrap3d) {
      pain::backend::enable3d();
      renderAPI.m_renderer3d.beginScene(currentTime, wrap3d->first,
                                        wrap3d->second);
      worldScene.renderSystems(pain::RenderPass::Dim3d, renderAPI, currentTime);
      renderAPI.m_renderer3d.endScene();
    }
    if (wrap2d) {
      pain::backend::disable3d();
      renderAPI.m_renderer2d.beginScene();
      // HACK: Temporary using render api directly inside scripts
      worldScene.renderSystems(pain::RenderPass::Dim2d, renderAPI, currentTime);
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

} // namespace tcc
