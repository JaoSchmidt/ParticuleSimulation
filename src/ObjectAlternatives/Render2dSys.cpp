/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ObjectAlternatives/Render2dSys.h"

#include "CoreRender/Renderer/RenderApi.h"
#include "Debugging/Profiling.h"

namespace obj::Systems
{

namespace
{
/// Equivalent to Render2dSys.h
template <typename ObjectT, bool Rotated>
void submit(pain::Renderer2d &renderer2d, const ObjectT &obj)
{
  float rotation = 0.f;
  if constexpr (Rotated)
    rotation = obj.m_rotationRadians;
  std::visit(
      [&](auto &shape) {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (Rotated) {
          if constexpr (std::is_same_v<T, pain::QuadShape>) {
            renderer2d.submitQuad(obj.m_position, shape.side, rotation,
                                  obj.m_layer, *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::RectShape>) {
            renderer2d.submitRect(obj.m_position, shape.size, rotation,
                                  obj.m_layer, *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
            renderer2d.submitTri(obj.m_position, {shape.base, shape.height},
                                 rotation, obj.m_layer, *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::LineShape>) {
            renderer2d.submitLine(obj.m_position, shape.destination,
                                  shape.thickness, obj.m_layer,
                                  *obj.m_material);
          }
        } else {
          if constexpr (std::is_same_v<T, pain::QuadShape>) {
            renderer2d.submitQuad(obj.m_position, shape.side, obj.m_layer,
                                  *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::RectShape>) {
            renderer2d.submitRect(obj.m_position, shape.size, obj.m_layer,
                                  *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
            renderer2d.submitTri(obj.m_position, {shape.base, shape.height},
                                 obj.m_layer, *obj.m_material);
          } else if constexpr (std::is_same_v<T, pain::LineShape>) {
            renderer2d.submitLine(obj.m_position, shape.destination,
                                  shape.thickness, obj.m_layer,
                                  *obj.m_material);
          }
        }
      },
      obj.m_size);
}
} // namespace

void Render2d::onRender(pain::RenderApi &renderApi,
                        const std::vector<Asteroid::Object> &asteroids,
                        const std::vector<Wall::Object> &walls)
{
  PROFILE_FUNCTION();
  pain::Renderer2d &renderer2d = renderApi.m_renderer2d;
  {
    // The ECS asteroids own a RotationComponent, so the system reaches them
    // through its rotation query.
    PROFILE_SCOPE("ParticleSimulationObj::onRender - rotation quads");
    for (const auto &asteroid : asteroids)
      submit<decltype(asteroid), true>(renderer2d, asteroid);
  }
  {
    // The walls have no rotation member, so the system reaches them through
    // its texture query, which submits without the rotation argument.
    PROFILE_SCOPE("ParticleSimulationObj::onRender - texture quads");
    for (const auto &wall : walls)
      submit<decltype(wall), false>(renderer2d, wall);
  }
  // The Render2d system is still registered in this mode, so it keeps
  // rendering the ECS leftovers (camera grid, mouse pointer) and flushing the
  // RenderContext. Nothing to replicate here.
}

} // namespace obj::Systems
