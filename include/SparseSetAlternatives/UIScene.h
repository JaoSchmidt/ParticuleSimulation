/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "AlternativeScene.h"
namespace tcc
{

/**
 * @brief Scene specialization dedicated to user interface logic and ImGui
 *        driven components.
 *
 * SparseSetUIScene is a thin wrapper over AlternativeScene using the
 * pain::UIComponents registry configuration. It centralizes:
 *  - UI entity management.
 *  - ImGui native script binding and lifetime management.
 *  - UI-specific systems and rendering flow.
 *
 * This scene is intended to remain independent from gameplay / world logic
 * and may coexist alongside other scene types (e.g. world scenes) with
 * isolated registries and systems.
 */
class SparseSetUIScene : public AlternativeScene<pain::UIComponents>
{
  template <typename... Ts> struct AreAllTagsRegistered;
  template <typename... Ts> struct AreAllTagsRegistered<TypeList<Ts...>> {
    static constexpr bool value = pain::UIComponents::allRegistered<Ts...>();
  };

public:
  using AlternativeScene<pain::UIComponents>::AlternativeScene;

  /**
   * @brief Creates a SparseSetUIScene instance with the required engine
   * services.
   *
   * @param eventDispatcher Shared engine event dispatcher.
   * @param solState Shared Lua state.
   * @param threadPool Shared thread pool.
   * @return Newly constructed SparseSetUIScene.
   */
  static SparseSetUIScene create(reg::EventDispatcher &eventDispatcher,
                                 ThreadPool &threadPool);

  /**
   * @brief Registers a system into the scene with compile-time validation.
   * System will be executed during game loop IN ORDER they are added
   *
   * System must:
   *  - be constructible
   *  - inherit the class Systems<pain::UIComponents>
   *  - have at least one system interface: IOnUpdate, IOnEvent, IOnRender
   *  - use components registered inside pain::UIComponents
   *
   * If the system already exists, insertion is ignored and a warning is logged.
   *
   * @tparam Sys System type.
   * @tparam Args Constructor argument types.
   * @param args Arguments forwarded to the system constructor.
   */
  template <typename Sys, typename... Args>
    requires std::is_constructible_v<Sys,
                                     reg::ArcheRegistry<pain::UIComponents> &,
                                     reg::EventDispatcher &, Args...> &&
             std::constructible_from<Sys,
                                     reg::ArcheRegistry<pain::UIComponents> &,
                                     reg::EventDispatcher &, Args...> &&
             ValidSystem<Sys> && AreAllTagsRegistered<typename Sys::Tags>::value
  void addSystem(Args &&...args)
  {
    auto [itSystem, isInserted] = m_systems.emplace(
        std::make_pair(std::type_index(typeid(Sys)), //
                       std::make_unique<Sys>(m_registry, m_eventDispatcher,
                                             std::forward<Args>(args)...) //
                       ));
    if (!isInserted) {
      PLOG_W("Could not insert System {}", typeid(Sys).name());
      return;
    }
    Sys *s = static_cast<Sys *>(itSystem->second.get());
    if constexpr (std::derived_from<Sys, IOnEvent>)
      m_eventSystems.emplace_back(s);
    if constexpr (std::derived_from<Sys, IOnRender>)
      m_renderSystems[static_cast<size_t>(s->getRenderPass())].emplace_back(s);
    if constexpr (std::derived_from<Sys, IOnUpdate>)
      m_updateSystems.emplace_back(s);
  }
};

} // namespace tcc
