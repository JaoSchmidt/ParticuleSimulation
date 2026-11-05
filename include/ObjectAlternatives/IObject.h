/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file IObject.h
 * @brief The interface every plain-object system iterates through.
 *
 * The systems only know this interface, so each access goes through the heap
 * pointer and the vtable. This is the polymorphic layout the ECS is compared
 * against.
 */

#pragma once

#include <glm/vec2.hpp>
#include <memory>
#include <pain.h>
#include <vector>

namespace pain
{
struct Renderer2d;
} // namespace pain

namespace obj
{

class IObject
{
public:
  virtual ~IObject() = default;

  // Kinematics: objects that don't move keep these no-ops
  virtual void updateRotation() {}
  virtual void move(float dtSeconds) { UNUSED(dtSeconds) }

  // Render2d
  virtual void render(pain::Renderer2d &renderer) const = 0;

  // Collision
  virtual glm::vec2 &position() = 0;
  virtual glm::vec2 offset() const = 0;
  virtual const pain::ColliderVariant &shape() const = 0;
  virtual bool isTrigger() const = 0;
  /// @return The object's velocity, or nullptr if it is static.
  virtual glm::vec2 *velocity() { return nullptr; }
};

/// Every object of the simulation. Each one has its own heap allocation.
using ObjectList = std::vector<std::unique_ptr<IObject>>;

} // namespace obj
