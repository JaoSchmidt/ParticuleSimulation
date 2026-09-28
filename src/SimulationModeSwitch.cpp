#include "SimulationModeSwitch.h"

#include "CustomPanel.h"
#include "MousePointer.h"
#include "imgui.h"
#include <painless.h>

namespace
{
/// Only one switcher may own the panel, otherwise a restart would leave the
/// previous scene's simulation reachable from a live callback.
SimulationModeSwitch *s_activeSwitch = nullptr;
} // namespace

SimulationModeSwitch &SimulationModeSwitch::create(pain::Scene &scene,
                                                   pain::Application *app,
                                                   SimulationMode initialMode)
{
  if (s_activeSwitch != nullptr) {
    s_activeSwitch->detach();
    delete s_activeSwitch;
  }
  s_activeSwitch = new SimulationModeSwitch(scene, *app, initialMode);
  return *s_activeSwitch;
}

SimulationModeSwitch::SimulationModeSwitch(pain::Scene &scene,
                                           pain::Application &app,
                                           SimulationMode initialMode)
    : m_scene(scene), m_app(app), m_mode(initialMode)
{
  createAssets();
  m_simulation = &buildSimulation(m_mode);

  painless::customPanel::registerPanel("Simulation", 1.F,
                                       painless::InterfaceMenu::SIDEBAR);
  m_panelId = painless::customPanel::addToPanel("Simulation",
                                                [this]() { renderControls(); });
}

void SimulationModeSwitch::detach()
{
  if (m_panelId == -1)
    return;
  painless::customPanel::removeFromPanel("Simulation", m_panelId);
  m_panelId = -1;
}

void SimulationModeSwitch::createAssets()
{
  pain::RenderApi &renderAPI = m_app.getRenderApi();
  const pain::AppInit &cfg = m_app.getCurrentConfig();

  m_assets.m_camera = pain::Dummy2dCamera::createMovingCamera(
      m_scene, cfg.defaultWidth, cfg.defaultHeight, cfg.defaultZoom2d);

  m_assets.m_wallMaterial = &renderAPI.m_materialManager.createMaterial(
      {.name = "Wall Material", //
       .color = pain::Colors::PastelBlue,
       .shader = renderAPI.m_shaderManager.getDefaultShader(
           pain::DefaultShader::Texture)});

  // Asteroids ----------------------------------------------------------------
  pain::TextureSheet &asteroidSheet =
      pain::TextureManager::createTexSheetWithDivisions(
          "asteroids", "resources/textures/asteroid-lite.png", 7, 1,
          {{0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}} //
      );
  pain::Color asteroidColor = pain::Colors::PastelBlue;
  pain::Shader &asteroidShader =
      renderAPI.m_shaderManager.getDefaultShader(pain::DefaultShader::Texture);
  auto asteroidTex = [&asteroidSheet](unsigned short id) {
    bool useSimpleTex = false;
    if (useSimpleTex)
      return pain::TextureVariant{&pain::TextureManager::getDefaultTexture(
          pain::TextureManager::DefaultTexture::General)};
    return pain::TextureVariant{pain::SheetStruct{&asteroidSheet, id}};
  };

  // pain::Shader &asteroidShader =
  //     renderAPI.m_shaderManager.getDefaultShader(pain::DefaultShader::Circles);
  std::array<pain::Material *, 7> asteroidMaterials = {
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 1", //
           .color = asteroidColor,        //
           .shader = asteroidShader,
           .texture = asteroidTex(0)}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 2", //
           .color = asteroidColor,        //
           .shader = asteroidShader,
           .texture = asteroidTex(1)}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 3", //
           .color = asteroidColor,        //
           .shader = asteroidShader,
           .texture = asteroidTex(2)}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 4", //
           .color = asteroidColor,        //
           .shader = asteroidShader,
           .texture = asteroidTex(3)}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 5", //
           .color = asteroidColor,        //
           .shader = asteroidShader,
           .texture = asteroidTex(4)}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 6", //
           .color = asteroidColor,        //
           .shader = asteroidShader,
           .texture = asteroidTex(5)}),
      &renderAPI.m_materialManager.createMaterial(
          {.name = "Asteroid Material 7", //
           .color = asteroidColor,        //
           .shader = asteroidShader,
           .texture = asteroidTex(6)}) //
  };
  m_assets.m_asteroidMaterials.assign(asteroidMaterials.begin(),
                                      asteroidMaterials.end());

  // Walls ---------------------------------------------------------------------
  m_assets.m_walls = {{glm::vec2(0.F, 2.F), glm::vec2(10.F, 1.F)}, //
                      {glm::vec2(3.F, 0.F), glm::vec2(1.F, 10.F)},
                      {glm::vec2(-3.F, 0.F), glm::vec2(1.F, 10.F)},
                      {glm::vec2(0.F, -2.F), glm::vec2(10.F, 1.F)}};

  m_assets.m_mousePointer =
      MousePointer::create(m_scene, renderAPI, m_assets.m_camera);
}

ParticleSimulationBase &
SimulationModeSwitch::buildSimulation(SimulationMode mode)
{
  // The count is owned by the running flavour, so it has to be read through the
  // base: the outgoing and incoming flavours are different types and the
  // default is only right on the very first build.
  const int count = m_simulation != nullptr
                        ? m_simulation->asteroidCount()
                        : static_cast<int>(Simulation::asteroidAmount);
  switch (mode) {
  case SimulationMode::Ecs:
    return ParticleSimulationECS::createScriptScene(m_scene, m_assets, count);
  case SimulationMode::Object:
  default:
    return ParticleSimulationObj::createScriptScene(m_scene, m_assets, count);
  }
}

void SimulationModeSwitch::setMode(SimulationMode mode)
{
  if (mode == m_mode || m_simulation == nullptr)
    return;

  // Ask the outgoing flavour to take back its world entities while it is still
  // alive, then bind the new script over it. Both createScriptScene calls end
  // in Scene::emplaceScript, which replaces the NativeScriptComponent's
  // instance, so this assignment is what destroys m_simulation.
  m_simulation->releaseWorldResources();
  m_mode = mode;
  m_simulation = &buildSimulation(mode);
}

void SimulationModeSwitch::renderModeSelector()
{
  int mode = static_cast<int>(m_mode);
  if (ImGui::Combo("Mode", &mode, "Object\0ECS\0"))
    setMode(static_cast<SimulationMode>(mode));
}

void SimulationModeSwitch::renderControls()
{
  renderModeSelector();
  ImGui::Separator();
  m_simulation->renderControls();
}
