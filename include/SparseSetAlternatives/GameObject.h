#pragma once
#include "SparseSetAlternatives/WorldScene.h"

/** @brief SceneTraits specialization for the UI scene. */
template <> struct pain::SceneTraits<tcc::SparseSetScene> {
  /** @brief Checks whether a component type is registered in UIComponents. */
  template <typename T> static constexpr bool isRegistered()
  {
    return WorldComponents::isRegistered<T>();
  }

  /** @brief Checks whether all component types are registered in UIComponents.
   */
  template <typename... Ts> static constexpr bool allRegistered()
  {
    return WorldComponents::allRegistered<Ts...>();
  }
};

using SparseSetObject = pain::GameObject<tcc::SparseSetScene>;
