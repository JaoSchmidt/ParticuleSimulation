#include <pain.h>
#include <painless.h>

#include "CustomPanel.h"
#include "OrthoCamera.h"
#include "ParticleSimulation.h"
#include "aliasesSystems.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

namespace
{
/// @brief Creates the camera, the materials and the wall geometry: everything
///        all simulations share.
SimulationAssets createSimulationAssets(pain::Scene &scene,
                                        pain::RenderApi &renderAPI,
                                        const pain::AppInit &cfg)
{
  SimulationAssets assets;

  assets.m_camera = DummyCamera::createStaticCamera(
      scene, cfg.defaultWidth, cfg.defaultHeight, cfg.defaultZoom2d);

  assets.m_wallMaterial = &renderAPI.m_materialManager.createMaterial(
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

  /// Each asterioid has its own material, they share the shader, but differ in
  /// texture
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
  assets.m_asteroidMaterials.assign(asteroidMaterials.begin(),
                                    asteroidMaterials.end());

  // Walls ---------------------------------------------------------------------
  assets.m_walls = {{glm::vec2(0.F, 4.F), glm::vec2(20.F, 1.F)}, //
                    {glm::vec2(6.F, 0.F), glm::vec2(1.F, 20.F)},
                    {glm::vec2(-6.F, 0.F), glm::vec2(1.F, 20.F)},
                    {glm::vec2(0.F, -4.F), glm::vec2(20.F, 1.F)}};

  return assets;
}
} // namespace

pain::Application *pain::createApplication()
{
  // Retrieve the context the player will alter when using the launcher
  IniConfig ini;
  ini.readAndUpdate();

  // Retrieve the app context defined inside "resources/InternalConfig.ini"
  InternalConfig internalIni;
  internalIni.readAndUpdate(ini.assetsPath.value);
  // TODO: create a function to translate init directly to AppInit
  // Create the application + OpenGL + Event contexts
  Application *app = Application::createApplication( //
      {
          .title = internalIni.title.get().c_str(),     //
          .defaultWidth = ini.defaultWidth.get(),       //
          .defaultHeight = ini.defaultHeight.get(),     //
          .defaultZoom2d = internalIni.zoomLevel.get(), //
          .fullWindow = ini.fullwindow.get(),           //
          .fullScreen = ini.fullscreen.get(),           //
          .gridCellSize = internalIni.gridSize.get(),
      },
      {.swapChainTarget = internalIni.swapChainTarget.get()} //
  );

  // Create the ECS World Scene and its systems

  ParticleSimulationBase *simulation = nullptr;
  if constexpr (Simulation::mode == SimulationMode::Archetype) {
    LOG_I("Using Archetypes registries");
    pain::Scene &scene = Sys::declareArchetypeSystems(app);
    SimulationAssets assets = createSimulationAssets(scene, app->getRenderApi(),
                                                     app->getCurrentConfig());
    simulation = &ParticleSimulationArchetype::createScriptScene(
        scene, assets, static_cast<int>(Simulation::asteroidAmount));
  } else {
    LOG_I("Using Object Mode");
    pain::Scene &scene = Sys::declareObj(app);
    SimulationAssets assets = createSimulationAssets(scene, app->getRenderApi(),
                                                     app->getCurrentConfig());
    simulation = &ParticleSimulationObj::createScriptScene(
        scene, assets, static_cast<int>(Simulation::asteroidAmount));
  }
  // The simulation owns the "Simulation" panel so its controls can be tweaked
  // at runtime without changing the flavour that is compiled in.
  painless::customPanel::registerPanel("Simulation", 1.F,
                                       painless::InterfaceMenu::SIDEBAR);
  painless::customPanel::addToPanel(
      "Simulation", [simulation]() { simulation->renderControls(); });

  // (Optional) Creating the ECS UI scene
  UIScene &uiScene = app->createUIScene();
  // (Optional) A small native script that works as our game engine editor
  painless::Editor &editor = painless::Editor::create(uiScene, *app);

  // (Optional) Define a small native script for the world scene
  // that will be executed on as root script. Must have added
  // System::NativeScript
  return app;
}

#ifdef PLATFORM_IS_LINUX
int main(int argc, char *argv[])
#elif defined PLATFORM_IS_WINDOWS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine,
                   int nCmdShow)
#endif
{
  bool isSettingsGuiNeeded = pain::Pain::initiateIni();
  EndGameFlags flags;
  flags.restartGame = !isSettingsGuiNeeded;
  if (isSettingsGuiNeeded) {
    pain::Application *app = painless::createLauncher();
    flags = pain::Pain::runAndDeleteApplication(app);
  }
  if constexpr (Simulation::mode == SimulationMode::Archetype ||
                Simulation::mode == SimulationMode::Object)
    while (flags.restartGame) {
      pain::Application *app = pain::createApplication();
      flags = pain::Pain::runAndDeleteApplication(app);
    }
  return 0;
}
