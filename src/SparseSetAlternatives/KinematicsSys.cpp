/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "SparseSetAlternatives/KinematicsSys.h"
#include "Debugging/Profiling.h"
#include "Physics/Movement3dComponent.h"
#include "Physics/MovementComponent.h"
#include "Physics/RotationComponent.h"
namespace tcc::Systems
{
namespace detail
{
void _implOnUpdate(
    pain::DeltaTime deltaTime, reg::View<pain::RotationComponent> viewRotation,
    reg::View<pain::Transform2dComponent, pain::Movement2dComponent>
        viewMovement2d,
    reg::View<pain::Transform3dComponent, pain::Movement3dComponent>
        viewMovement3d)
{
  PROFILE_FUNCTION();
  // =============================================================== //
  // Update Rotation Components
  // =============================================================== //
  {
    PROFILE_SCOPE("Scene::updateSystems - rotation");
    // Was: one ChunkView per archetype, each walked with an index over the
    // SoA columns. A single dense array per component now, so the same loop
    // body becomes the lambda given to forEach.
    viewRotation.forEach([](pain::RotationComponent &r) {
      r.m_rotation = {cos(glm::radians(r.m_rotationRadians)),
                      sin(glm::radians(r.m_rotationRadians)), 0};
    });
  }

  // =============================================================== //
  // Update Movement Components
  // =============================================================== //
  {
    PROFILE_SCOPE("Scene::updateSystems - movement");
    float dt = deltaTime.getSecondsf();

    viewMovement2d.forEach(
        [dt](pain::Transform2dComponent &t, pain::Movement2dComponent &m) {
          t.m_position += m.m_velocity * dt;
        });
  }
  {
    PROFILE_SCOPE("Scene::updateSystems - movement");
    float dt = deltaTime.getSecondsf();

    viewMovement3d.forEach(
        [dt](pain::Transform3dComponent &t, pain::Movement3dComponent &m) {
          t.m_position += m.m_velocity * dt;
        });
  }
}
} // namespace detail

} // namespace tcc::Systems
