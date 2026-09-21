#include "Wall.h"
#include <pain.h>

reg::Entity Wall::create(pain::Scene &scene, const glm::vec2 &pos,
                         const glm::vec2 &size, pain::Material &m)
{
  reg::Entity entity = scene.createEntity();
  scene.createComponents(
      entity,                          //
      pain::Transform2dComponent{pos}, //
      pain::SpriteComponent::create({.shape = pain::RectShape{size}}),
      pain::MaterialComponent{m}, //
      pain::SAPCollider::createAABB(size));
  return entity;
}
