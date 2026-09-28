#include <pain.h>
#include <painless.h>

#include "SimulationModeSwitch.h"
#include "aliasesSystems.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>

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
  pain::Scene &scene = Sys::declareSystems(app);

  // The switcher owns the shared assets, starts the default flavour and takes
  // over the "Simulation" panel, from where the other one can be selected.
  SimulationModeSwitch::create(scene, app, SimulationModeSwitch::defaultMode);

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
  while (flags.restartGame) {
    pain::Application *app = pain::createApplication();
    flags = pain::Pain::runAndDeleteApplication(app);
  }
  return 0;
}
