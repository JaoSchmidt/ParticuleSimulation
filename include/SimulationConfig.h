#pragma once

/**
 * @file SimulationConfig.h
 * @brief Single source of truth for the settings of every simulation.
 *
 * SIM_MODE comes from the CMake option of the same name, which is also where
 * its default lives. CMake translates the option to the numeric values of
 * SimulationMode.
 */

#include "Assets/RandNumberGenerator.h"

#ifndef SIM_MODE
#error "SIM_MODE must be defined by CMake (see the SIM_MODE option)"
#endif

static_assert(SIM_MODE >= 0 && SIM_MODE <= 2,
              "SIM_MODE must be 0 (object), 1 (archetype) or 2 (sparse set)");

/** @brief Which implementation of the simulation is running. */
enum class SimulationMode {
  Object = 0, ///< Plain C++ objects, stepped and drawn by the script itself.
  Archetype =
      1, ///< Entities and components, stepped by the registered systems.
  SparseSet = 2
};

namespace Simulation
{
/// The implementation this target was configured with.
inline constexpr SimulationMode mode = static_cast<SimulationMode>(SIM_MODE);

// Particle simulation (main.cpp) ---------------------------------------------
// A single seed drives the same Gaussian draw sequence in every mode, so
// asteroid i spawns at the same spot with the same velocity no matter which
// implementation is running, and no matter how many asteroids are asked for.
inline constexpr pain::RNG::SeedType asteroidSeed = 1;
inline constexpr float asteroidRadius = 0.1f;
inline constexpr unsigned asteroidAmount = 100;
/// Upper bound of the amount slider. Collision is O(n^2) here, so this is
/// deliberately modest.
inline constexpr int maxAsteroidCount = 2000;

// Big entity simulation (main-big-entity.cpp) --------------------------------
inline constexpr int bigEntityStartCount = 1024;
/// Upper bound of the amount input and of the profiling rounds: 128 MiB of
/// data.
inline constexpr int bigEntityMaxCount = 131072;
} // namespace Simulation
