#include "Wall.h"
#include <pain.h>

reg::Entity Wall::createECS(pain::Scene &scene, const glm::vec2 &pos,
                            const glm::vec2 &size, pain::Material &m)
{
  reg::Entity entity = scene.createEntity();
  scene.createComponents( //
      entity,             //
      cmp::Pos2d{pos},    //
      cmp::Sprite::create({.shape = pain::RectShape{size}}),
      cmp::Material{m}, //
      cmp::Collider::createAABB(size));
  return entity;
}

Wall::Object Wall::createObject(const glm::vec2 &pos, const glm::vec2 &size,
                                pain::Material &m)
{
  return Wall::Object{
      .m_position = pos,
      .m_size = pain::RectShape{size},
      .m_material = &m,
      // Collider
      .m_offset = {0.0F, 0.0F},
      .m_shape{pain::AABBShape{size * 0.5f}},
      .m_isTrigger{false} //
  };
}
