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
#include "oneflow/core/job/graph_scope_vars.h"

namespace oneflow {

namespace {

std::atomic<bool>* GetGraphVerboseStepLr() {
  static std::atomic<bool> graph_verbose_step_lr{false};
  return &graph_verbose_step_lr;
}

int32_t* GetGraphDebugMaxPyStackDepthVarPtr() {
  static thread_local int32_t graph_debug_max_py_stack_depth = 2;
  return &graph_debug_max_py_stack_depth;
}

bool* GetGraphDebugModeFlagPtr() {
  static thread_local bool graph_debug_mode_flag = false;
  return &graph_debug_mode_flag;
}

}  // namespace

bool IsOpenGraphVerboseStepLr() {
  auto* graph_verbose_step_lr = GetGraphVerboseStepLr();
  bool is_graph_verbose_step_lr = *graph_verbose_step_lr;
  return is_graph_verbose_step_lr;
}

void SetGraphVerboseStepLr(bool verbose) {
  auto* graph_verbose_step_lr = GetGraphVerboseStepLr();
  *graph_verbose_step_lr = verbose;
}

int32_t GetGraphDebugMaxPyStackDepthVar() {
  return *GetGraphDebugMaxPyStackDepthVarPtr();
}

void SetGraphDebugMaxPyStackDepthVar(int32_t depth) {
  *GetGraphDebugMaxPyStackDepthVarPtr() = depth; 
}


bool GetGraphDebugModeFlag() {
  return *GetGraphDebugModeFlagPtr();
}

void SetGraphDebugModeFlag(bool flag) {
  *GetGraphDebugModeFlagPtr() = flag;
}
}  // namespace oneflow
