
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "ECS/Components/NativeScript.h"
#include "SparseSetAlternatives/AlternativeScene.h"
namespace tcc
{

/**
 * @brief Scene specialization dedicated to gameplay / world simulation.
 *
 * Scene is a specialization of AlternativeScene using the pain::WorldComponents
 * registry configuration. It manages:
 *  - World entities and gameplay components.
 *  - Native (C++) script binding and lifecycle.
 *  - World systems constrained to valid component tags.
 *
 * This scene represents the main simulation layer of the engine and is
 * intentionally isolated from UI-specific logic.
 */
class SparseSetScene : public AlternativeScene<pain::WorldComponents>
{
  template <typename... Ts> struct AreAllTagsRegistered;
  template <typename... Ts> struct AreAllTagsRegistered<TypeList<Ts...>> {
    static constexpr bool value = pain::WorldComponents::allRegistered<Ts...>();
  };

public:
  using AlternativeScene<pain::WorldComponents>::AlternativeScene;

  /**
   * @brief Creates a Scene instance with the required engine services.
   *
   * @param eventDispatcher Shared engine event dispatcher.
   * @param solState Shared Lua state.
   * @param threadPool Shared thread pool.
   * @return Newly constructed Scene.
   */
  static SparseSetScene create(reg::EventDispatcher &eventDispatcher,
                               ThreadPool &threadPool);

  // =============================================================== //
  // NATIVE SCRIPTING RELATED
  // =============================================================== //

  /**
   * @brief Retrieves a bound native script instance from an entity.
   *
   * The entity must own a pain::NativeScriptComponent and its instance must be
   * of type S.
   *
   * @tparam S Expected script type.
   * @param entity Target entity.
   * @return Reference to the script instance.
   */
  template <typename S>
    requires(pain::WorldComponents::isRegistered<pain::NativeScriptComponent>())
  S &getNativeScript(reg::Entity entity)
  {
    pain::NativeScriptComponent &nsc =
        getComponent<pain::NativeScriptComponent>(entity);
    return static_cast<S &>(*nsc.instance);
  }

  /**
   * @brief Binds and initializes an already constructed native script instance.
   *
   * The script object is moved into the pain::NativeScriptComponent and its
   * onCreate callback is executed if available.
   *
   * @tparam N Script type.
   * @param entity Target entity.
   * @param scene Target scene.
   * @param n Script instance to move.
   * @return Reference to the bound script instance.
   */
  template <typename N>
    requires(pain::WorldComponents::isRegistered<pain::NativeScriptComponent>())
  static N &emplaceScript(reg::Entity entity, SparseSetScene &scene, N &&n)
  {
    pain::NativeScriptComponent &nsc =
        scene.getComponent<pain::NativeScriptComponent>(entity);
    nsc.bindAndInitiate<N>(std::forward<N>(n));
    if (nsc.instance && nsc.onCreateFunction)
      nsc.onCreateFunction(nsc.instance.get());
    return static_cast<N &>(*nsc.instance);
  }

  /**
   * @brief Constructs and binds a native script directly inside the component.
   *
   * The script is constructed using the provided arguments and immediately
   * bound to the entity's pain::NativeScriptComponent. The onCreate callback is
   * executed if available.
   *
   * @tparam N Script type.
   * @tparam Args Constructor argument types.
   * @param entity Target entity.
   * @param scene Target scene.
   * @param args Arguments forwarded to the script constructor.
   * @return Reference to the constructed script instance.
   */
  template <typename N, typename... Args>
    requires std::constructible_from<N, reg::Entity, SparseSetScene &,
                                     Args...> &&
             (pain::WorldComponents::isRegistered<
                 pain::NativeScriptComponent>())
  static N &emplaceScript(reg::Entity entity, SparseSetScene &scene,
                          Args &&...args)
  {
    pain::NativeScriptComponent &nsc =
        scene.getComponent<pain::NativeScriptComponent>(entity);
    nsc.bindAndEmplace<N>(entity, scene, std::forward<Args>(args)...);
    if (nsc.instance && nsc.onCreateFunction)
      nsc.onCreateFunction(nsc.instance.get());
    return static_cast<N &>(*nsc.instance);
  }
};

} // namespace tcc
