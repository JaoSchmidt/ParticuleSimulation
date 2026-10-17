/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// Scene.cpp
#include "SparseSetAlternatives/AlternativeScene.h"

#include "Debugging/Profiling.h"

namespace tcc
{

// =============================================================== //
// =============================================================== //
// MULTI THREAD RELATED
// =============================================================== //
// =============================================================== //
template <reg::CompileTimeBitMaskType Manager>
void AlternativeScene<Manager>::enqueueMainThread(MainThreadJob job)
{
  std::lock_guard lock(m_mainThreadMutex);
  m_mainThreadJobs.push(std::move(job));
}

template <reg::CompileTimeBitMaskType Manager>
void AlternativeScene<Manager>::flushMainThreadJobs()
{
  std::queue<MainThreadJob> jobs;

  {
    std::lock_guard lock(m_mainThreadMutex);
    std::swap(jobs, m_mainThreadJobs);
  }

  while (!jobs.empty()) {
    jobs.front()();
    jobs.pop();
  }
}

// =============================================================== //
// Constructors
// =============================================================== //

template <reg::CompileTimeBitMaskType Manager>
AlternativeScene<Manager>::AlternativeScene(reg::EventDispatcher &ed,
                                            ThreadPool &threadPool)
    : m_registry(), m_entity(createEntity()), m_threadPool(threadPool),
      m_eventDispatcher(ed) {};

template <reg::CompileTimeBitMaskType Manager>
void AlternativeScene<Manager>::updateSystems(pain::DeltaTime deltaTime)
{
  PROFILE_FUNCTION();
  flushMainThreadJobs();
  for (auto *sys : m_updateSystems)
    static_cast<IOnUpdate *>(sys)->onUpdate(deltaTime);
  m_eventDispatcher.update();
}

template <reg::CompileTimeBitMaskType Manager>
void AlternativeScene<Manager>::updateSystems(const SDL_Event &event)
{
  PROFILE_SCOPE("Scene::updateSystems - updates for native scripts");
  for (auto *sys : m_eventSystems)
    static_cast<IOnEvent *>(sys)->onEvent(event);
}
template <reg::CompileTimeBitMaskType Manager>
void AlternativeScene<Manager>::renderSystems(pain::RenderPass pass,
                                              pain::RenderApi &renderAPI,

                                              pain::DeltaTime currentTime)
{
  PROFILE_SCOPE("Scene::renderSystems - rendering native scripts");
  for (auto *sys : m_renderSystems[static_cast<uint8_t>(pass)])
    static_cast<IOnRender *>(sys)->onRender(renderAPI, currentTime);
}

template class AlternativeScene<pain::WorldComponents>;
template class AlternativeScene<pain::UIComponents>;

} // namespace tcc
