#pragma once
#include <pain.h>
#include <painless.h>

class MainScript : public pain::WorldObject
{

public:
  static MainScript &createScriptScene(pain::Scene &scene,
                                       pain::Application *app);
  MainScript(reg::Entity entity, pain::Scene &scene,
             std::vector<reg::Entity> &&stars, reg::Entity orthocamera,
             std::vector<reg::Entity> &&asteroid,
             std::vector<reg::Entity> &&walls, reg::Entity mp);

  std::vector<std::vector<int>> m_backgroundMap;
  std::shared_ptr<pain::Shader> m_texture_shader;
  reg::Entity m_mousePointer;
  reg::Entity m_orthoCamera;
  std::vector<reg::Entity> m_stars;
  std::vector<reg::Entity> m_asteroids;
  std::vector<reg::Entity> m_walls;
  // Player m_player;
  const static unsigned starAmout = 0;
  const static unsigned asteroidAmount = 20;
};
