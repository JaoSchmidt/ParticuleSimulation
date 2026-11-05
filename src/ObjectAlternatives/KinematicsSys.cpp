/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ObjectAlternatives/KinematicsSys.h"

#include "Debugging/Profiling.h"

namespace obj::Systems
{

void Kinematics::onUpdate(ObjectList &objects, pain::DeltaTime deltaTime)
{
  PROFILE_SCOPE("Scene::udpateSystems - Kinematics");
  {
    // Mirrors the ECS RotationComponent update
    PROFILE_SCOPE("Scene::updateSystems - rotation");
    for (auto &object : objects)
      object->updateRotation();
  }
  {
    PROFILE_SCOPE("Scene::updateSystems - movement");
    const float dtf = deltaTime.getSecondsf();
    for (auto &object : objects)
      object->move(dtf);
  }
}

} // namespace obj::Systems
