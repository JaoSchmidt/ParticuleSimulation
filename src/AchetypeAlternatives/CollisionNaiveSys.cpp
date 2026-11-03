/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ArchetypeAlternatives/CollisionNaiveSys.h"

#include "Debugging/Profiling.h"
#include "Physics/Collision/CollisionNaiveSys.h"

namespace pain
{
namespace Systems
{

namespace
{
// Flattened handle to one entity of an archetype chunk. The pointers borrow
// memory owned by the archetype's component arrays, which are not touched for
// the duration of the update.
struct DynamicEntity {
  Transform2dComponent *transform;
  ColliderComponent *collider;
  Movement2dComponent *movement;
  reg::Entity entity;
};
struct StaticEntity {
  Transform2dComponent *transform;
  ColliderComponent *collider;
  reg::Entity entity;
};
} // namespace
#if 0
void archaltDetail::_implOnUpdate(
    std::vector<reg::ChunkView<Transform2dComponent, ColliderComponent>>
        chunksStatic,
    std::vector<reg::ChunkView<Transform2dComponent, ColliderComponent,
                               Movement2dComponent>>
        chunks,
    reg::EventDispatcher &eventDispatcher)
{
  PROFILE_SCOPE("Scene::updateSystems - collision detection")

  // Materialise every matching chunk into the same flat arrays the sparse set
  // alternative builds from its views. Both pairwise passes below need random
  // access (the moving pass starts its inner loop at i + 1 to skip self
  // comparisons and duplicate unordered pairs).
  std::vector<DynamicEntity> dynamic;
  for (auto &chunk : chunks) {
    auto *__restrict t = std::get<0>(chunk.arrays);
    auto *__restrict c = std::get<1>(chunk.arrays);
    auto *__restrict m = std::get<2>(chunk.arrays);
    for (size_t i = 0; i < chunk.count; ++i)
      dynamic.push_back({&t[i], &c[i], &m[i], chunk.entities[i]});
  }

  std::vector<StaticEntity> statics;
  for (auto &chunk : chunksStatic) {
    auto *__restrict t = std::get<0>(chunk.arrays);
    auto *__restrict c = std::get<1>(chunk.arrays);
    for (size_t i = 0; i < chunk.count; ++i)
      statics.push_back({&t[i], &c[i], chunk.entities[i]});
  }

  {
    PROFILE_SCOPE(
        "Scene::updateSystems - collision detection vs moving objects");
    for (size_t i = 0; i < dynamic.size(); ++i) {
      // NOTE: start at i + 1 to avoid an entity colliding with itself and to
      // avoid testing each unordered pair twice.
      for (size_t j = i + 1; j < dynamic.size(); ++j) {
        detail::narrowPhaseCollision( //
            *dynamic[i].transform,    //
            *dynamic[i].collider,     //
            *dynamic[i].movement,     //
            dynamic[i].entity,        //
            *dynamic[j].transform,    //
            *dynamic[j].collider,     //
            *dynamic[j].movement,     //
            dynamic[j].entity,        //
            eventDispatcher);
      }
    }
  }
  {
    PROFILE_SCOPE(
        "Scene::updateSystems - collision detection vs static objects");

    for (size_t i = 0; i < dynamic.size(); ++i) {
      for (size_t j = 0; j < statics.size(); ++j) {
        detail::narrowPhaseCollisionStatic( //
            *dynamic[i].transform,          //
            *dynamic[i].collider,           //
            *dynamic[i].movement,           //
            dynamic[i].entity,              //
            *statics[j].transform,          //
            *statics[j].collider,           //
            statics[j].entity,              //
            eventDispatcher);
      }
    }
  }
}
#endif
} // namespace Systems
} // namespace pain
