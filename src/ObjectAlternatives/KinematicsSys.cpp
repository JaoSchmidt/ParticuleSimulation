/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ObjectAlternatives/KinematicsSys.h"

#include "Debugging/Profiling.h"
#include <cmath>

namespace obj::Systems
{

void Kinematics::onUpdate(std::vector<Asteroid::Object> &asteroids,
                          pain::DeltaTime deltaTime)
{
  PROFILE_SCOPE("Scene::udpateSystems - Kinematics");
  {
    // Mirrors the ECS RotationComponent update: keep the axis vector in sync
    // with the angle stored on the object.
    PROFILE_SCOPE("Scene::updateSystems - rotation");
    for (auto &asteroid : asteroids) {
      asteroid.m_rotation = {
          std::cos(glm::radians(asteroid.m_rotationRadians)),
          std::sin(glm::radians(asteroid.m_rotationRadians)), 0.F};
    }
  }
  {
    PROFILE_SCOPE("Scene::updateSystems - movement");
    const float dtf = deltaTime.getSecondsf();
    for (auto &asteroid : asteroids) {
      asteroid.m_position += asteroid.m_velocity * dtf;
    }
  }
}

} // namespace obj::Systems
