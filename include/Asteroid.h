// Asteroid.h
#pragma once

#include <glm/vec2.hpp>
#include <pain.h>

namespace Asteroid
{
reg::Entity create(pain::Scene &scene, pain::Material &m, glm::vec2 pos,
                   glm::vec2 vel, float radius);
} // namespace Asteroid
