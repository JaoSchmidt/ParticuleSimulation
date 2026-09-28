#pragma once
#include "ParticleSimulation.h"
#include <pain.h>

/**
 * @brief Owns the "Simulation" editor panel and the currently running flavour.
 *
 * The whole point of the project is to compare the two implementations under
 * identical conditions, so they share one camera, one mouse pointer, one set
 * of textures and materials, and one asteroid count. The only thing a switch
 * changes is which simulation script is bound to the scene: the mode
 * independent assets in SimulationAssets are built once and handed to
 * whichever flavour is created, and the outgoing flavour is asked to give
 * back whatever it put in the world registry before its script is replaced.
 *
 * Both simulations live as native scripts on the scene's root entity, owned by
 * its NativeScriptComponent. Binding the other one over it destroys the
 * previous instance, which is what makes the swap a single assignment rather
 * than any manual teardown.
 */
class SimulationModeSwitch
{

public:
  /** @brief The flavour a fresh run starts in. */
  static constexpr SimulationMode defaultMode = SimulationMode::Object;

  /**
   * @brief Builds the shared assets, starts @p initialMode and takes over the
   *        "Simulation" panel.
   *
   * Calling this again (the app supports restarting) detaches the previous
   * switcher's panel and replaces it, so a stale scene pointer is never
   * reachable from a panel callback.
   */
  static SimulationModeSwitch &create(pain::Scene &scene,
                                      pain::Application *app,
                                      SimulationMode initialMode);

  /** @brief Tears the panel down so no callback can reach this object again. */
  void detach();

  /** @brief Swaps the running flavour, keeping the shared assets and count. */
  void setMode(SimulationMode mode);
  SimulationMode getMode() const { return m_mode; }

  /** @brief Contents of the "Simulation" panel. */
  void renderControls();

private:
  SimulationModeSwitch(pain::Scene &scene, pain::Application &app,
                       SimulationMode initialMode);

  /// @brief Creates the camera, the mouse pointer, the materials and the wall
  ///        geometry: everything both flavours share.
  void createAssets();
  /// @brief Builds the walls for @p mode and binds its script, replacing
  ///        whatever simulation is currently bound.
  ParticleSimulationBase &buildSimulation(SimulationMode mode);
  void renderModeSelector();

  pain::Scene &m_scene;
  pain::Application &m_app;
  SimulationAssets m_assets;
  SimulationMode m_mode = defaultMode;
  /// @brief The live simulation. Not owned: it belongs to the scene's root
  ///        NativeScriptComponent and is destroyed by the next bind.
  ParticleSimulationBase *m_simulation = nullptr;
  int m_panelId = -1;
};
