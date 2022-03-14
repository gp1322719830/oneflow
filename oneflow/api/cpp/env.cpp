/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <glog/logging.h>
#include "oneflow/api/cpp/env.h"
#include "oneflow/api/cpp/env_impl.h"
#include "oneflow/core/framework/shut_down_util.h"
#include "oneflow/core/thread/thread_consistent_id.h"

namespace oneflow_api {
void initialize() {
  if (of::Global<OneFlowEnv>::Get() == nullptr) { of::Global<OneFlowEnv>::New(); }
  of::SetShuttingDown(false);
}

void release() {
  if (of::Global<OneFlowEnv>::Get() != nullptr) { of::Global<OneFlowEnv>::Delete(); }
  of::SetShuttingDown();
  of::ResetThisThreadUniqueConsistentId().GetOrThrow();
}

}  // namespace oneflow_api