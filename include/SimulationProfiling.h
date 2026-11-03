#pragma once

#include "Core.h"
#ifdef PROFILING

#include <filesystem>
#include <string>

/**
 * @brief Helpers shared by all simulation flavours to start the profiler.
 */
namespace SimulationProfiling
{
/// @brief Builds a unique stem from the simulation name and asteroid count.
inline std::string generateProfileFilename(const std::string &simulationName,
                                           int asteroidCount)
{
  return simulationName + "-" + std::to_string(asteroidCount) + ".O" +
         std::to_string(pain::optimizationLevel);
}

/// @brief Returns "stem.json", or "stem-N.json" if that file already exists.
inline std::string getNextAvailableFileName(const std::string &baseStem)
{
  namespace fs = std::filesystem;

  const std::string extension = ".json";
  std::string name = baseStem + extension;
  int counter = 1;

  while (fs::exists(name)) {
    name = baseStem + "-" + std::to_string(counter++) + extension;
  }

  return name;
}

/// @brief Frames to wait after a simulation start/reset before profiling.
constexpr int profileStartDelayFrames = 300;
/// @brief Number of frames captured before the stream closes automatically.
constexpr int profileFrameCount = 30;
} // namespace SimulationProfiling

#endif
