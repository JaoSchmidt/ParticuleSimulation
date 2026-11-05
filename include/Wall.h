#pragma once

#include "Core.h"
#include "ObjectAlternatives/IObject.h"
#include <memory>
#include <pain.h>

namespace Wall
{
reg::Entity createECS(pain::Scene &scene, const glm::vec2 &pos,
                      const glm::vec2 &size, pain::Material &m);

/**
 * @brief A non-ECS wall.
 *
 * Every field below stands in for a component of the ECS version, which is
 * what makes the two comparable: same data, same math, but stored in the
 * object instead of in archetype chunks. It never moves, so it keeps the
 * interface's no-op kinematics and has no velocity.
 */
class Object : public obj::IObject
{
public:
  Object(const glm::vec2 &pos, const glm::vec2 &size, pain::Material &m);

  void render(pain::Renderer2d &renderer) const override;
  glm::vec2 &position() override { return m_position; }
  glm::vec2 offset() const override { return m_offset; }
  const pain::ColliderVariant &shape() const override { return m_shape; }
  bool isTrigger() const override { return m_isTrigger; }

private:
  glm::vec2 m_position{0.F, 0.F}; ///< Transform2dComponent
  pain::ShapeVariant m_size = pain::RectShape{{0.F, 0.F}}; ///< SpriteComponent
  const pain::Material *m_material = nullptr;       ///< MaterialComponent
  pain::RenderLayer m_layer = pain::RenderLayer::C; ///< SpriteComponent
  // Collider members
  glm::vec2 m_offset{0.0F, 0.0F};
  pain::ColliderVariant m_shape{pain::AABBShape{}};
  bool m_isTrigger{false};
};

/**
 * @brief Creates a wall as a plain C++ object.
 */
std::unique_ptr<Object> createObject(const glm::vec2 &pos,
                                     const glm::vec2 &size, pain::Material &m);

}; // namespace Wall
