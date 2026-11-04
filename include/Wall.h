#pragma once

#include "Core.h"
#include <pain.h>

namespace Wall
{
reg::Entity createECS(pain::Scene &scene, const glm::vec2 &pos,
                      const glm::vec2 &size, pain::Material &m);

/**
 * @brief A non-ECS asteroid.
 *
 * Every field below stands in for a component of the ECS version, which is
 * what makes the two comparable: same data, same math, but stored in the
 * object instead of in archetype chunks, and stepped by this object instead
 * of by a system.
 */
struct Object {
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
 * @brief Creates an Wall as a plain C++ object.
 */
Object createObject(const glm::vec2 &pos, const glm::vec2 &size,
                    pain::Material &m);

}; // namespace Wall
