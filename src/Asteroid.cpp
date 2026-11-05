// Asteroid.cpp
#include "Asteroid.h"
#include "CoreRender/Renderer/RenderApi.h"
#include <cmath>
#include <pain.h>

reg::Entity Asteroid::createECS(pain::Scene &scene, pain::Material &m,
                                glm::vec2 pos, glm::vec2 vel, float radius)
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

Asteroid::Object::Object(pain::Material &m, glm::vec2 pos, glm::vec2 vel,
                         float radius)
    : m_position(pos), m_velocity(vel),
      m_size(pain::RectShape{glm::vec2(radius)}), m_material(&m),
      m_shape(pain::AABBShape{glm::vec2(radius) * 0.5f})
{
}

void Asteroid::Object::updateRotation()
{
  // Mirrors the ECS RotationComponent update: keep the axis vector in sync with
  // the angle stored on the object.
  m_rotation = {std::cos(glm::radians(m_rotationRadians)),
                std::sin(glm::radians(m_rotationRadians)), 0.F};
}

void Asteroid::Object::move(float dtSeconds)
{
  m_position += m_velocity * dtSeconds;
}

void Asteroid::Object::render(pain::Renderer2d &renderer) const
{
  std::visit(
      [&](const auto &shape) {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<T, pain::QuadShape>) {
          renderer.submitQuad(m_position, shape.side, m_rotationRadians,
                              m_layer, *m_material);
        } else if constexpr (std::is_same_v<T, pain::RectShape>) {
          renderer.submitRect(m_position, shape.size, m_rotationRadians,
                              m_layer, *m_material);
        } else if constexpr (std::is_same_v<T, pain::TriangleShape>) {
          renderer.submitTri(m_position, {shape.base, shape.height},
                             m_rotationRadians, m_layer, *m_material);
        } else if constexpr (std::is_same_v<T, pain::LineShape>) {
          renderer.submitLine(m_position, shape.destination, shape.thickness,
                              m_layer, *m_material);
        }
      },
      m_size);
}

std::unique_ptr<Asteroid::Object> Asteroid::createObject(pain::Material &m,
                                                         glm::vec2 pos,
                                                         glm::vec2 vel,
                                                         float radius)
{
  return std::make_unique<Object>(m, pos, vel, radius);
}
