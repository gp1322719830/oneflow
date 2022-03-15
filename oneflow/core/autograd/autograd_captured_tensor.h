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
#ifndef ONEFLOW_CORE_AUTOGRAD_AUTOGRAD_CAPTURED_TENSOR_H_
#define ONEFLOW_CORE_AUTOGRAD_AUTOGRAD_CAPTURED_TENSOR_H_

#include "oneflow/core/framework/functional/functional.h"
#include "oneflow/core/framework/tensor.h"

namespace oneflow {
namespace one {

class AutogradCapturedTensor final : public ProxyTensor<AutogradCapturedTensor> {
 public:
  AutogradCapturedTensor(const std::shared_ptr<Tensor>& tensor)
      : ProxyTensor<AutogradCapturedTensor>(tensor->detach().GetPtrOrThrow()) {
    CHECK_JUST(this->tensor_->set_requires_grad(tensor->requires_grad()));
    CHECK(tensor->grad_fn_node()) << "The grad function node is expected for the captured tensor";
    grad_fn_node_ = &tensor->mut_grad_fn_node();
  }

  std::shared_ptr<const FunctionNode> grad_fn_node() const override { return *grad_fn_node_; }
  void set_grad_fn_node(const std::shared_ptr<FunctionNode>& grad_fn_node) override {
    PRINT_BUG_PROMPT_AND_ABORT();
  }
  const std::shared_ptr<FunctionNode>& mut_grad_fn_node() override { return *grad_fn_node_; }

  std::shared_ptr<Tensor> contiguous() const override {
    const auto& tensor = std::const_pointer_cast<Tensor>(shared_from_this());
    if (tensor_->is_contiguous()) { return tensor; }
    return CHECK_JUST(functional::ToContiguous(tensor));
  }

 private:
  const std::shared_ptr<FunctionNode>* grad_fn_node_;
};

}  // namespace one
}  // namespace oneflow

#endif  // ONEFLOW_CORE_AUTOGRAD_AUTOGRAD_CAPTURED_TENSOR_H_
