// Asteroid.cpp
#include "Asteroid.h"
#include <pain.h>

reg::Entity Asteroid::create(pain::Scene &scene, pain::Material &m,
                             glm::vec2 pos, glm::vec2 vel, float radius)
{
  // assuming size of quad sprite = diameter
  reg::Entity entity = scene.createEntity();
  scene.createComponents(entity, pain::Transform2dComponent{pos}, //
                         pain::Movement2dComponent{vel},          //
                         pain::RotationComponent{},               //
                         pain::MaterialComponent{m},              //
                         pain::SAPCollider::createCircle(radius));
  return entity;
}
