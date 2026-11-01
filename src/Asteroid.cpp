// Asteroid.cpp
#include "Asteroid.h"
#include "SparseSetAlternatives/WorldScene.h"
#include <pain.h>

template <typename SceneT>
reg::Entity Asteroid::createECS(SceneT &scene, pain::Material &m, glm::vec2 pos,
                                glm::vec2 vel, float radius)
{
  // assuming size of quad sprite = diameter
  reg::Entity entity = scene.createEntity();
  scene.createComponents(                                 //
      entity, pain::Transform2dComponent{pos},            //
      cmp::Mov2d{vel},                                    //
      cmp::Rot{},                                         //
      cmp::Sprite::create(                                //
          {.shape = pain::RectShape{glm::vec2(radius)}}), //
      cmp::Material{m},                                   //
      cmp::Collider::createAABB(glm::vec2(radius))        //
  );
  return entity;
}

template reg::Entity
Asteroid::createECS<pain::Scene>(pain::Scene &scene, pain::Material &m,
                                 glm::vec2 pos, glm::vec2 vel, float radius);
template reg::Entity Asteroid::createECS<tcc::SparseSetScene>(
    tcc::SparseSetScene &scene, pain::Material &m, glm::vec2 pos,
    glm::vec2 vel, float radius);

Asteroid::Object Asteroid::createObject(pain::Material &m, glm::vec2 pos,
                                        glm::vec2 vel, float radius)
{
  return Asteroid::Object{
      .m_position = pos,
      .m_velocity = vel,
      .m_size = pain::RectShape{glm::vec2(radius)},
      .m_material = &m,
      // Collider
      .m_offset = {0.0F, 0.0F},
      .m_shape{pain::AABBShape{glm::vec2(radius) * 0.5f}},
      .m_isTrigger{false} //
  };
}
