#include "Stars.h"
#include <pain.h>

namespace Stars
{
reg::Entity create(pain::Scene &scene, pain::Material &mat, glm::vec2 &pos)
{
  reg::Entity entity = scene.createEntity();
  scene.createComponents(entity,
                         pain::Transform2dComponent{glm::vec3(pos, 0.f)}, //
                         pain::MaterialComponent{mat},                    //
                         pain::SpriteComponent{});
  return entity;
}

} // namespace Stars
