/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "SparseSetAlternatives/WorldScene.h"

namespace tcc
{

SparseSetScene SparseSetScene::create(reg::EventDispatcher &eventDispatcher,
                                      ThreadPool &threadPool)
{
  return SparseSetScene(eventDispatcher, threadPool);
}

} // namespace tcc
