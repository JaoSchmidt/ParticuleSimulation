/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// RenderSys.cpp
#include "ArchetypeAlternatives/Render2dSys.h"
#include "CoreRender/MaterialComponent.h"
#include "CoreRender/Renderer/RenderApi.h"
#include "CoreRender/SpriteComponent.h"
#include "Debugging/Profiling.h"
#include "Physics/MovementComponent.h"
#include "Physics/RotationComponent.h"

namespace tcc
{
namespace Systems::detail
{
// NOTE: pay attention that the systems are communicating directly with the
// renderAPI, meaning the state of the API (opengl, vulkan, directx) NEEDS to be
// programmed to render the batch at any moment, including the transparency,
// depth, dual face, etc
// This means you probl. need to call like backend::enableXX() from here

// constexpr std::array<SphereDivision, 3> sphereDivisions = {
//     SphereDivision::D_8x8, SphereDivision::D_16x16, SphereDivision::D_32x32};

// =============================================================== //
// Render Components
// =============================================================== //
void _implOnRender(
    pain::RenderApi &renderer,
    const std::vector<reg::ChunkViewConst<
        const pain::Transform2dComponent, const pain::SpriteComponent,
        const pain::RotationComponent, const pain::MaterialComponent>>
        &chunksRotation,
    const std::vector<reg::ChunkViewConst<
        const pain::Transform2dComponent, const pain::SpriteComponent,
        const pain::MaterialComponent>> &chunksTexture)
{
  pain::Renderer2d &renderer2d = renderer.m_renderer2d;
  {
    for (const auto &chunk : chunksRotation) {
      const auto [t, s, r, m] = chunk.arrays;
      for (size_t i = 0; i < chunk.count; ++i) {
        std::visit(
            [&](const auto &shape) {
              using T = std::decay_t<decltype(shape)>;
              if constexpr (std::is_same_v<T, pain::QuadShape>) {
                renderer2d.submitQuad(t[i].m_position, shape.side,
                                      r[i].m_rotationRadians, s[i].layer,
                                      *m[i]);
              } else if constexpr (std::is_same_v<T, pain::RectShape>) {
                renderer2d.submitRect(t[i].m_position, shape.size,
                                      r[i].m_rotationRadians, s[i].layer,
                                      *m[i]);
              } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
                renderer2d.submitTri(t[i].m_position,
                                     {shape.base, shape.height},
                                     r[i].m_rotationRadians, s[i].layer, *m[i]);
              } else if constexpr (std::is_same_v<T, pain::LineShape>) {
                renderer2d.submitLine(t[i].m_position, shape.destination,
                                      shape.thickness, s[i].layer, *m[i]);
              }
            },
            s[i].m_shape);
      }
    }
  }
  {
    for (const auto &chunk : chunksTexture) {
      const auto [t, s, m] = chunk.arrays;
      for (size_t i = 0; i < chunk.count; ++i) {
        std::visit(
            [&](const auto &shape) {
              using T = std::decay_t<decltype(shape)>;
              if constexpr (std::is_same_v<T, pain::QuadShape>) {
                renderer2d.submitQuad(t[i].m_position, shape.side, s[i].layer,
                                      *m[i]);
              } else if constexpr (std::is_same_v<T, pain::RectShape>) {
                renderer2d.submitRect(t[i].m_position, shape.size, s[i].layer,
                                      *m[i]);
              } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
                renderer2d.submitTri(t[i].m_position,
                                     {shape.base, shape.height}, s[i].layer,
                                     *m[i]);
              } else if constexpr (std::is_same_v<T, pain::LineShape>) {
                renderer2d.submitLine(t[i].m_position, shape.destination,
                                      shape.thickness, s[i].layer, *m[i]);
              }
            },
            s[i].m_shape);
      }
    }
  }
}

} // namespace Systems::detail
} // namespace tcc
