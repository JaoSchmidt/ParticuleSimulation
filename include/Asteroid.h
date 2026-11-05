/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// Asteroid.h
#pragma once

#include "ObjectAlternatives/IObject.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <pain.h>

/**
 * @brief Two implementations of the same asteroid, for an ECS vs. plain
 *        object comparison.
 *
 * createECS() spreads the asteroid state over components. createObject() keeps
 * the very same state inline in a C++ object. Which one is used is decided at
 * compile time by Simulation::mode.
 */
namespace Asteroid
{
/**
 * @brief Creates an asteroid as an ECS entity.
 */
reg::Entity createECS(pain::Scene &scene, pain::Material &m, glm::vec2 pos,
                      glm::vec2 vel, float radius);

/**
 * @brief A non-ECS asteroid.
 *
 * Every field below stands in for a component of the ECS version, which is
 * what makes the two comparable: same data, same math, but stored in the
 * object instead of in archetype chunks, and stepped by this object instead
 * of by a system.
 */
class Object : public obj::IObject
{
public:
  Object(pain::Material &m, glm::vec2 pos, glm::vec2 vel, float radius);

  void updateRotation() override;
  void move(float dtSeconds) override;
  void render(pain::Renderer2d &renderer) const override;
  glm::vec2 &position() override { return m_position; }
  glm::vec2 offset() const override { return m_offset; }
  const pain::ColliderVariant &shape() const override { return m_shape; }
  bool isTrigger() const override { return m_isTrigger; }
  glm::vec2 *velocity() override { return &m_velocity; }

private:
  glm::vec2 m_position{0.F, 0.F}; ///< Transform2dComponent
  glm::vec2 m_velocity{0.F, 0.F}; ///< Movement2dComponent
  pain::ShapeVariant m_size = pain::RectShape{{0.F, 0.F}}; ///< SpriteComponent
  float m_rotationRadians = 0.F;                    ///< RotationComponent
  glm::vec3 m_rotation{0.F, 1.F, 0.F};              ///< RotationComponent
  const pain::Material *m_material = nullptr;       ///< MaterialComponent
  pain::RenderLayer m_layer = pain::RenderLayer::C; ///< SpriteComponent
  // Collider members
  glm::vec2 m_offset{0.0F, 0.0F};
  pain::ColliderVariant m_shape{pain::AABBShape{}};
  bool m_isTrigger{false};
};

/**
 * @brief Creates an asteroid as a plain C++ object.
 */
std::unique_ptr<Object> createObject(pain::Material &m, glm::vec2 pos,
                                     glm::vec2 vel, float radius);

} // namespace Asteroid
