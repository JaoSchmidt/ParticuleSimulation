/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/**
 * @file NativeScriptComponent.h
 * @brief Sparse set alternative of pain::NativeScriptComponent.
 *
 * The engine component stores a pain::GameObject<pain::Scene> as its type
 * erased script, so it can only ever own scripts written against the archetype
 * scene. This component is byte for byte the same contract - one owned script
 * instance plus five optional callback pointers - but its Scriptable is
 * SparseSetObject, i.e. pain::GameObject<tcc::SparseSetScene>, so scripts bound
 * to a SparseSetScene store a reference to the right scene type.
 *
 * It reuses pain::tag::NativeScript, which is what WorldComponents maps
 * to a bit, so the sparse world scene accepts it in every component list the
 * archetype component is accepted in. Only one of the two types may be used
 * per scene.
 *
 * The callbacks are plain function pointers rather than virtual calls, exactly
 * as in the engine component: presence of each callback is detected at compile
 * time from the bound script type through the concepts in Concepts.h.
 *
 * @note This component lives in namespace tcc, so it can coexist with
 * pain::NativeScriptComponent inside the same binary.
 */

#pragma once
#include "Assets/DeltaTime.h"
#include "ECS/Components/ComponentManager.h"
#include "Scripting/Native/Concepts.h"
#include <SDL2/SDL_events.h>
#include <memory>

namespace pain
{
template <typename SceneT> class GameObject;
}
namespace tcc
{
class SparseSetScene;

/**
 * @struct NativeScriptComponent
 * @brief Owns and executes a native C++ script bound to a SparseSetScene.
 *
 * Scripts are validated at compile time by checkScriptMethods<T>() when they
 * are bound, so an incorrectly named or wrongly typed callback is rejected at
 * compile time rather than silently skipped at runtime.
 *
 * The component destroys its script instance when it is destroyed, moved from
 * or rebound, which is what lets a scene swap its root script with a single
 * assignment.
 */
struct NativeScriptComponent {
private:
  /** @brief Type erased handle the callbacks dispatch through. */
  using Scriptable = pain::GameObject<SparseSetScene>;

public:
  /// @brief ECS component tag used for compile-time registration.
  using tag = tag::NativeScript;

  /**
   * @brief Pointer to the bound script instance (type-erased).
   *
   * Lifetime is owned by the component. The instance is destroyed automatically
   * when the component is destroyed or reassigned.
   */
  std::unique_ptr<Scriptable, void (*)(Scriptable *)> instance = {nullptr,
                                                                  nullptr};

  /// @name Script lifecycle function pointers
  ///@{
  void (*onCreateFunction)(Scriptable *) = nullptr;
  void (*onDestroyFunction)(Scriptable *) = nullptr;
  void (*onRenderFunction)(Scriptable *, pain::RenderApi &,
                           pain::DeltaTime) = nullptr;
  void (*onUpdateFunction)(Scriptable *, pain::DeltaTime) = nullptr;
  void (*onEventFunction)(Scriptable *, const SDL_Event &) = nullptr;
  ///@}

  /**
   * @brief Bind and initialize a script instance by constructing it in place.
   *
   * @tparam T Script type.
   * @tparam Args Constructor argument types.
   * @param args Arguments forwarded to the script constructor.
   */
  template <typename T, typename... Args> void bindAndEmplace(Args &&...args)
  {
    checkScriptMethods<T>();
    static_assert(std::is_constructible_v<T, Args...>,
                  "Error: You are binding a script whose constructor doesn't "
                  "accept (Entity, SparseSetScene&, Args...). Perhaps you are "
                  "missing `using SparseSetObject::SparseSetObject;`?");

    instance = {new T(std::forward<Args>(args)...),
                [](Scriptable *instance) { //
                  delete static_cast<T *>(instance);
                }};

    if constexpr (hasOnCreateMethod<T>) {
      onCreateFunction = [](Scriptable *instance) {
        static_cast<T *>(instance)->onCreate();
      };
    } else {
      onCreateFunction = nullptr;
    }

    if constexpr (hasOnDestroyMethod<T>) {
      onDestroyFunction = [](Scriptable *instance) {
        static_cast<T *>(instance)->onDestroy();
      };
    } else {
      onDestroyFunction = nullptr;
    }

    if constexpr (hasOnRenderMethod<T>) {
      onRenderFunction = [](Scriptable *instance,
                            pain::RenderApi &renderContext,
                            pain::DeltaTime realTime) {
        static_cast<T *>(instance)->onRender(renderContext, realTime);
      };
    } else {
      onRenderFunction = nullptr;
    }

    if constexpr (hasOnUpdateMethod<T>) {
      onUpdateFunction = [](Scriptable *instance, pain::DeltaTime deltaTime) {
        static_cast<T *>(instance)->onUpdate(deltaTime);
      };
    } else {
      onUpdateFunction = nullptr;
    }

    if constexpr (hasOnEventMethod<T>) {
      onEventFunction = [](Scriptable *instance, const SDL_Event &event) {
        static_cast<T *>(instance)->onEvent(event);
      };
    } else {
      onEventFunction = nullptr;
    }
  }

  /**
   * @brief Bind and initialize an already constructed script instance.
   *
   * @tparam T Script type, deduced from @p t.
   * @param t Script instance to move into this component.
   */
  template <typename T> void bindAndInitiate(T &&t)
  {
    bindAndEmplace<std::remove_reference_t<T>>(std::forward<T>(t));
  }

  NativeScriptComponent() = default;
  NativeScriptComponent(const NativeScriptComponent &) = delete;
  NativeScriptComponent &operator=(const NativeScriptComponent &) = delete;
  /// @brief Destroys the bound script instance if present.
  ~NativeScriptComponent() = default;

  /**
   * @brief Move constructor.
   *
   * Transfers ownership and clears the source component to avoid a double
   * deletion.
   */
  NativeScriptComponent(NativeScriptComponent &&other) noexcept
      : instance(std::move(other.instance)),
        onCreateFunction(other.onCreateFunction),
        onDestroyFunction(other.onDestroyFunction),
        onRenderFunction(other.onRenderFunction),
        onUpdateFunction(other.onUpdateFunction),
        onEventFunction(other.onEventFunction)
  {
    other.instance = nullptr;
    other.onCreateFunction = nullptr;
    other.onDestroyFunction = nullptr;
    other.onRenderFunction = nullptr;
    other.onUpdateFunction = nullptr;
    other.onEventFunction = nullptr;
  }

  /**
   * @brief Move assignment operator.
   *
   * Transfers ownership and all callback bindings, leaving the source empty.
   */
  NativeScriptComponent &operator=(NativeScriptComponent &&other) noexcept
  {
    if (this != &other) {
      instance = std::move(other.instance);
      onCreateFunction = other.onCreateFunction;
      onDestroyFunction = other.onDestroyFunction;
      onRenderFunction = other.onRenderFunction;
      onUpdateFunction = other.onUpdateFunction;
      onEventFunction = other.onEventFunction;

      other.instance = nullptr;
      other.onCreateFunction = nullptr;
      other.onDestroyFunction = nullptr;
      other.onRenderFunction = nullptr;
      other.onUpdateFunction = nullptr;
      other.onEventFunction = nullptr;
    }
    return *this;
  }
};

} // namespace tcc
