
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "OrthoCamera.h"

namespace DummyCamera
{
template <typename SceneT>
reg::Entity createStaticCamera(SceneT &scene, int resolutionWidth,
                               int resolutionHeight, float zoomLevel,
                               glm::vec2 center)
{
  reg::Entity entity = scene.createEntity("Static2DCamera");

  scene.createComponents(
      entity, pain::Transform2dComponent{center},
      pain::OrthoCameraComponent::create(true, resolutionWidth,
                                         resolutionHeight, zoomLevel, entity));

  return entity;
}

template reg::Entity createStaticCamera(pain::Scene &, int, int, float,
                                        glm::vec2);

} // namespace DummyCamera
