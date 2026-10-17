/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// Render2dSys.cpp
#include "SparseSetAlternatives/Render2dSys.h"
#include "CoreRender/ColorIndexComponent.h"
#include "CoreRender/MaterialComponent.h"
#include "CoreRender/RenderContext.h"
#include "CoreRender/Renderer/RenderApi.h"
#include "CoreRender/SpriteComponent.h"
#include "Debugging/Profiling.h"
#include "Physics/MovementComponent.h"
#include "Physics/RotationComponent.h"
#include "TextComponent.h"

namespace tcc
{
namespace Systems::detail
{
// NOTE: pay attention that the systems are communicating directly with the
// renderAPI, meaning the state of the API (opengl, vulkan, directx) NEEDS to be
// programmed to render the batch at any moment, including the transparency,
// depth, dual face, etc
// This means you probl. need to call like backend::enableXX() from here

// =============================================================== //
// Render Components
// =============================================================== //
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
    reg::View<pain::Transform2dComponent, pain::TextComponent> viewText)
{
  UNUSED(currentTime)
  PROFILE_FUNCTION();
  pain::Renderer2d &renderer2d = renderer.m_renderer2d;
  {
    PROFILE_SCOPE("Scene::renderSystems - rotation quads");

    viewRotation.forEach([&](const pain::Transform2dComponent &t,
                             const pain::SpriteComponent &s,
                             const pain::RotationComponent &r,
                             const pain::MaterialComponent &m) {
      std::visit(
          [&](auto &shape) {
            using T = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<T, pain::QuadShape>) {
              renderer2d.submitQuad(t.m_position, shape.side,
                                    r.m_rotationRadians, s.layer, *m);
            } else if constexpr (std::is_same_v<T, pain::RectShape>) {
              renderer2d.submitRect(t.m_position, shape.size,
                                    r.m_rotationRadians, s.layer, *m);
            } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
              renderer2d.submitTri(t.m_position, {shape.base, shape.height},
                                   r.m_rotationRadians, s.layer, *m);
            } else if constexpr (std::is_same_v<T, pain::LineShape>) {
              renderer2d.submitLine(t.m_position, shape.destination,
                                    shape.thickness, s.layer, *m);
            }
          },
          s.m_shape);
    });
  }
  {
    PROFILE_SCOPE("Scene::renderSystems - texture quads");
    viewTexture.forEach([&](const pain::Transform2dComponent &t,
                            const pain::SpriteComponent &s,
                            const pain::MaterialComponent &m) {
      std::visit(
          [&](auto &shape) {
            using T = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<T, pain::QuadShape>) {
              renderer2d.submitQuad(t.m_position, shape.side, s.layer, *m);
            } else if constexpr (std::is_same_v<T, pain::RectShape>) {
              renderer2d.submitRect(t.m_position, shape.size, s.layer, *m);
            } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
              renderer2d.submitTri(t.m_position, {shape.base, shape.height},
                                   s.layer, *m);
            } else if constexpr (std::is_same_v<T, pain::LineShape>) {
              renderer2d.submitLine(t.m_position, shape.destination,
                                    shape.thickness, s.layer, *m);
            }
          },
          s.m_shape);
    });
  }
  {
    PROFILE_SCOPE("Scene::renderSystems - colored primitives");
    viewColored.forEach([&](const pain::Transform2dComponent &t,
                            const pain::SpriteComponent &s,
                            const pain::ColorIndexComponent &c,
                            const pain::MaterialComponent &m) {
      std::visit(
          [&](auto &shape) {
            using T = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<T, pain::QuadShape>) {
              renderer2d.submitQuad(t.m_position, shape.side, s.layer, *m,
                                    c.color);
            } else if constexpr (std::is_same_v<T, pain::RectShape>) {
              renderer2d.submitRect(t.m_position, shape.size, s.layer, *m,
                                    c.color);
            } else if constexpr (std::is_same_v<T, pain::LineShape>) {
              renderer2d.submitLine(t.m_position, shape.destination,
                                    shape.thickness, s.layer, *m, c.color);
            }
          },
          s.m_shape);
    });
  }
  {
    PROFILE_SCOPE("Scene::renderSystems - colored primitives");
    viewText.forEach(
        [&](const pain::Transform2dComponent &p, const pain::TextComponent &t) {
          renderer2d.submitString(p.m_position, t.scale, t.text, t.font,
                                  t.color, t.align);
        });
  }
  {
    PROFILE_SCOPE("Scene::renderSystems - scripts");
    const auto &commands = renderer.m_renderContext.getCommands();
    for (const auto &cmd : commands) {
      switch (cmd.m_type) {
      case pain::RenderCommandType::Quad:
        renderer2d.submitQuad(           //
            cmd.m_data.sprite.transform, //
            cmd.m_data.sprite.layer,     //
            *cmd.m_data.sprite.material);
        break;
      case pain::RenderCommandType::Triangle:
        renderer2d.submitTri(            //
            cmd.m_data.sprite.transform, //
            cmd.m_data.sprite.layer,     //
            *cmd.m_data.sprite.material);
        break;
      case pain::RenderCommandType::Text:
        renderer2d.submitString(      //
            cmd.m_data.text.position, //
            cmd.m_data.text.scale,    //
            cmd.m_data.text.string,   //
            *cmd.m_data.text.font,    //
            cmd.m_data.text.color);
        break;
      case pain::RenderCommandType::Rect:
        renderer2d.submitRect(           //
            cmd.m_data.sprite.transform, //
            cmd.m_data.sprite.layer,     //
            *cmd.m_data.sprite.material);
        break;
      case pain::RenderCommandType::Line:
        renderer2d.submitLine(           //
            cmd.m_data.line.origin,      //
            cmd.m_data.line.destination, //
            cmd.m_data.line.thickness,   //
            cmd.m_data.line.layer,       //
            *cmd.m_data.sprite.material);
        break;

      default:
        break;
      }
    }
  }
}

} // namespace Systems::detail
} // namespace tcc
