#include "Wall.h"
#include "CoreRender/Renderer/RenderApi.h"
#include <pain.h>

reg::Entity Wall::createECS(pain::Scene &scene, const glm::vec2 &pos,
                            const glm::vec2 &size, pain::Material &m)
{
  reg::Entity entity = scene.createEntity();
  scene.createComponents( //
      entity,             //
      cmp::Pos2d{pos},    //
      cmp::Sprite::create({.shape = pain::RectShape{size}}),
      cmp::Material{m}, //
      cmp::Collider::createAABB(size));
  return entity;
}

Wall::Object::Object(const glm::vec2 &pos, const glm::vec2 &size,
                     pain::Material &m)
    : m_position(pos), m_size(pain::RectShape{size}), m_material(&m),
      m_shape(pain::AABBShape{size * 0.5f})
{
}

void Wall::Object::render(pain::Renderer2d &renderer) const
{
  std::visit(
      [&](const auto &shape) {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<T, pain::QuadShape>) {
          renderer.submitQuad(m_position, shape.side, m_layer, *m_material);
        } else if constexpr (std::is_same_v<T, pain::RectShape>) {
          renderer.submitRect(m_position, shape.size, m_layer, *m_material);
        } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
          renderer.submitTri(m_position, {shape.base, shape.height}, m_layer,
                             *m_material);
        } else if constexpr (std::is_same_v<T, pain::LineShape>) {
          renderer.submitLine(m_position, shape.destination, shape.thickness,
                              m_layer, *m_material);
        }
      },
      m_size);
}

std::unique_ptr<Wall::Object> Wall::createObject(const glm::vec2 &pos,
                                                 const glm::vec2 &size,
                                                 pain::Material &m)
{
  return std::make_unique<Object>(pos, size, m);
}
