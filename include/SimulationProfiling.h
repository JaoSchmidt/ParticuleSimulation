#pragma once

#ifdef PROFILING

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>

/**
 * @brief Helpers shared by all simulation flavours to drive the profiler with
 * the space key.
 */
namespace SimulationProfiling
{
/// @brief Builds a prefix + local timestamp stem, e.g. "profile-2026-10-31_12-00-00".
inline std::string generateTimestampedFilename(const std::string &prefix)
{
  std::ostringstream oss;
  auto now = std::chrono::system_clock::now();
  auto inTime = std::chrono::system_clock::to_time_t(now);
  oss << prefix << std::put_time(std::localtime(&inTime), "%Y-%m-%d_%H-%M-%S");
  return oss.str();
}

/// @brief Returns the first "stem.nN.json" name that does not exist yet.
inline std::string getNextAvailableFileName(const std::string &baseStem)
{
  namespace fs = std::filesystem;

  std::string extension = ".json";
  int counter = 1;
  std::string name;

  do {
    name = baseStem + ".n" + std::to_string(counter++) + extension;
  } while (fs::exists(name));

  return name;
}

/// @brief Number of frames captured before the stream closes automatically.
constexpr int profileFrameCount = 30;
} // namespace SimulationProfiling

#endif
