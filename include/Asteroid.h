/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// Asteroid.h
#pragma once

#include <glm/vec2.hpp>
#include <pain.h>

/**
 * @brief Two implementations of the same asteroid, for an ECS vs. plain
 *        object comparison.
 *
 * createECS() is implemented in Asteroid-ECS.cpp and spreads the asteroid
 * state over components. createObject() is implemented in
 * Asteroid-Object.cpp and keeps the very same state inline in a C++ object.
 * Which one is used is decided at runtime by AsteroidMode.h.
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
struct Object {
  glm::vec2 m_position{0.F, 0.F}; ///< Transform2dComponent
  glm::vec2 m_velocity{0.F, 0.F}; ///< Movement2dComponent
  pain::ShapeVariant m_size = pain::RectShape{{0.F, 0.F}}; ///< SpriteComponent
  float m_rotationRadians = 0.F;                    ///< RotationComponent
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
Object createObject(pain::Material &m, glm::vec2 pos, glm::vec2 vel,
                    float radius);

} // namespace Asteroid
