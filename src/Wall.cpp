#include "Wall.h"
#include "SparseSetAlternatives/WorldScene.h"
#include <pain.h>

template <typename SceneT>
reg::Entity Wall::createECS(SceneT &scene, const glm::vec2 &pos,
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

template reg::Entity Wall::createECS<tcc::SparseSetScene>(
    tcc::SparseSetScene &scene, const glm::vec2 &pos, const glm::vec2 &size,
    pain::Material &m);
template reg::Entity Wall::createECS<pain::Scene>(pain::Scene &scene,
                                                  const glm::vec2 &pos,
                                                  const glm::vec2 &size,
                                                  pain::Material &m);

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
