/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ObjectAlternatives/KinematicsSys.h"

#include "Debugging/Profiling.h"

namespace obj::Systems
{

void Kinematics::onUpdate(std::vector<Asteroid::Object> &asteroids,
                          pain::DeltaTime deltaTime)
{
  PROFILE_SCOPE("ParticleSimulationObj - movement");
  // Non-ECS KinematicsSys. Walls have no movement, and nothing rotates the
  // asteroids' m_rotationRadians, so only the integration is left.
  const float dtf = deltaTime.getSecondsf();
  for (auto &asteroid : asteroids) {
    asteroid.m_position += asteroid.m_velocity * dtf;
  }
}

} // namespace obj::Systems
