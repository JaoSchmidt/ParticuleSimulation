#include "ParticleSimulation.h"
#include "Asteroid.h"
#include "Misc/Basic2dPlayer.h"
#include "MousePointer.h"
#include "Stars.h"
#include "Wall.h"
#include <pain.h>
#include <painless.h>

MainScript &MainScript::createScriptScene(pain::Scene &scene,
                                          pain::Application *app)
{
  pain::RenderApi &renderAPI = app->getRenderApi();
  pain::ShaderManager &shaderMan = renderAPI.m_shaderManager;
  const pain::AppInit &cfg = app->getCurrentConfig();
  // create the camera
  reg::Entity camEntity = pain::Dummy2dCamera::createMovingCamera(
      scene, cfg.defaultWidth, cfg.defaultHeight, cfg.defaultZoom2d);

  // STARS ---------------------------------------------------------------
  pain::TextureSheet &texSheet =
      pain::TextureManager::createTexSheetWithDivisions(
          "star_animation", // Unique name
          "resources/textures/star_animation_strip.png", 1, 4,
          {{0, 0}, {1, 0}, {2, 0}, {3, 0}});

  std::array<pain::Material *, 4> starsMaterials = {
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Star Material 1", //
           .shader =
               shaderMan.getDefaultShader(pain::DefaultShader::Instancing), //
           .texture = pain::SheetStruct{&texSheet, 0}}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Star Material 2", //
           .shader =
               shaderMan.getDefaultShader(pain::DefaultShader::Instancing), //
           .texture = pain::SheetStruct{&texSheet, 1}}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Star Material 3", //
           .shader =
               shaderMan.getDefaultShader(pain::DefaultShader::Instancing), //
           .texture = pain::SheetStruct{&texSheet, 2}}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Star Material 4", //
           .shader =
               shaderMan.getDefaultShader(pain::DefaultShader::Instancing), //
           .texture = pain::SheetStruct{&texSheet, 3}})                     //
  };
  std::vector<reg::Entity> stars;
  std::mt19937 gen(1337);
  std::uniform_real_distribution<float> dist(-1.f, 1.f);
  stars.reserve(starAmout);
  for (unsigned short i = 0; i < starAmout; i++) {
    glm::vec2 randomPos(dist(gen), dist(gen));
    stars.emplace_back(Stars::create(
        scene,
        *starsMaterials[static_cast<unsigned short>(i % texSheet.size())],
        randomPos));
  }

  // WALLS ---------------------------------------------------------------
  std::vector<reg::Entity> walls;
  // walls.emplace_back(scene, glm::vec2(0.0f, 0.0f), glm::vec2(0.2f, 6.2f));
  walls.reserve(5);
  walls.emplace_back(
      Wall::create(scene, glm::vec2(6.f, 4.f), glm::vec2(36.f, 1.f)));
  walls.emplace_back(
      Wall::create(scene, glm::vec2(0.f, 2.f), glm::vec2(36.f, 1.f)));
  walls.emplace_back(
      Wall::create(scene, glm::vec2(2.f, 0), glm::vec2(1.f, 36.f)));
  walls.emplace_back(
      Wall::create(scene, glm::vec2(-2.f, 0), glm::vec2(1.f, 36.f)));
  walls.emplace_back(
      Wall::create(scene, glm::vec2(0, -2.f), glm::vec2(36.f, 1.f)));
  // PLAYER ---------------------------------------------------------------
  // pain::Texture &shipTex =
  //     pain::resources::getTexture("resources/textures/ship_H.png");
  // Player player = {scene, shipTex};
  // auto lsc = &player.getComponent<pain::LuaScriptComponent>(scene);
  // lsc->bind("resources/scripts/lua_script.lua");
  // ASTEROID ---------------------------------------------------------------
  pain::TextureSheet &asteroidSheet =
      pain::TextureManager::createTexSheetWithDivisions(
          "asteroids", "resources/textures/asteroid.png", 1, 5,
          {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}});
  std::vector<reg::Entity> asteroids;
  asteroids.reserve(asteroidAmount);
  // for (short i = 0; i < asteroidAmount / 2; i++) {
  //   glm::vec2 randomPos(dist(gen), dist(gen));
  //   glm::vec2 randomVel(dist(gen), dist(gen));
  //   asteroids.emplace_back(Asteroid::create(          //
  //       scene,                                        //
  //       asteroidSheet,                                //
  //       static_cast<short>(i % asteroidSheet.size()), //
  //       std::move(randomPos),                         //
  //       std::move(randomVel), 0.1f));
  // }
  // for (short i = asteroidAmount / 2; i < asteroidAmount; i++) {
  //   glm::vec2 randomPos(dist(gen), dist(gen));
  //   glm::vec2 randomVel(dist(gen), dist(gen));
  //   asteroids.emplace_back(Asteroid::create(          //
  //       scene,                                        //
  //       asteroidSheet,                                //
  //       static_cast<short>(i % asteroidSheet.size()), //
  //       std::move(randomPos),                         //
  //       std::move(randomVel),                         //
  //       glm::vec2{0.2f, 0.2f}));                      //
  // }
  std::array<pain::Material *, 5> asteroidMaterials = {
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 1", //
           .shader =
               shaderMan.getDefaultShader(pain::DefaultShader::Instancing), //
           .texture = pain::SheetStruct{&texSheet, 0}}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 2", //
           .shader =
               shaderMan.getDefaultShader(pain::DefaultShader::Instancing), //
           .texture = pain::SheetStruct{&texSheet, 1}}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 3", //
           .shader =
               shaderMan.getDefaultShader(pain::DefaultShader::Instancing), //
           .texture = pain::SheetStruct{&texSheet, 2}}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 4", //
           .shader =
               shaderMan.getDefaultShader(pain::DefaultShader::Instancing), //
           .texture = pain::SheetStruct{&texSheet, 3}}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 5", //
           .shader =
               shaderMan.getDefaultShader(pain::DefaultShader::Instancing), //
           .texture = pain::SheetStruct{&texSheet, 5}})                     //
  };
  for (unsigned short i = 0; i < asteroidAmount / 2; i++) {
    asteroids.emplace_back(Asteroid::create(          //
        scene,                                        //
        *asteroidMaterials[i % asteroidSheet.size()], //
        glm::vec2{-1.f + 0.3f * i, 0.2f},             //
        glm::vec2{0.5f, 1.0f},                        //
        0.1f                                          //
        ));
    // glm::vec2{0.2f, 0.2f})); //
  }

  // ---------------------------------
  // add objects to collision System
  auto *s = scene.getSys<pain::Systems::SweepAndPruneSys>();
  s->insertColliders(asteroids);
  s->insertColliders(walls);

  // MOUSE POINTER
  // ---------------------------------------------------------------
  reg::Entity mp = MousePointer::create(scene, app->getRenderApi(), camEntity);

  return pain::Scene::emplaceScript<MainScript>(
      scene.getEntity(), scene, std::move(stars), camEntity,
      std::move(asteroids), std::move(walls), std::move(mp));
}

MainScript::MainScript(reg::Entity entity, pain::Scene &scene,
                       std::vector<reg::Entity> &&stars,
                       reg::Entity orthocamera,
                       std::vector<reg::Entity> &&asteroid,
                       std::vector<reg::Entity> &&walls, reg::Entity mp)
    : pain::WorldObject(entity, scene), m_mousePointer(std::move(mp)),
      m_orthoCamera(orthocamera), m_stars(std::move(stars)),
      m_asteroids(std::move(asteroid)), m_walls(std::move(walls)) {};
