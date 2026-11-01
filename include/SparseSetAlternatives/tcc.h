#pragma once
#include "SparseSetAlternatives/Application.h"
#include <pain.h>
namespace tcc
{

struct Tcc {

  /// @brief Runs an application, deletes it, and returns its end flags.
  static EndGameFlags runAndDeleteApplication(tcc::Application *app);
};

} // namespace tcc
