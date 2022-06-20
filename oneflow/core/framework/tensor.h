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
#ifndef ONEFLOW_CORE_FRAMEWORK_TENSOR_H_
#define ONEFLOW_CORE_FRAMEWORK_TENSOR_H_

#include <memory>
#include "oneflow/core/common/data_type.h"
#include "oneflow/core/common/shape_view.h"
#include "oneflow/core/common/shape.h"
#include "oneflow/core/common/stride.h"
#include "oneflow/core/memory/memory_case.pb.h"
#include "oneflow/core/framework/tensor_impl.h"
#include "oneflow/core/framework/transport_token.h"
#include "oneflow/core/common/error.h"

namespace oneflow {

class NdSbp;
class Device;

namespace one {

class FunctionNode;

class ConsistentTensor;
class MirroredTensor;

class Tensor : public std::enable_shared_from_this<Tensor> {
 public:
  virtual ~Tensor() = default;

  // Getters
  int64_t dim(int64_t index) const { return shape()->At(index); }
  int64_t nelement() const { return shape()->elem_cnt(); }
  int64_t ndim() const { return shape()->NumAxes(); }

  virtual std::shared_ptr<const Shape> shape() const = 0;
  virtual Symbol<DType> dtype() const = 0;
  virtual Maybe<TransportToken> transport_token() const = 0;
  virtual Maybe<Symbol<NdSbp>> nd_sbp() const = 0;
  virtual Maybe<Symbol<ParallelDesc>> parallel_desc() const = 0;
  virtual Maybe<Symbol<Device>> device() const = 0;
  virtual Maybe<Symbol<Device>*> mut_device() = 0;
  virtual bool is_cuda() const = 0;
  virtual bool is_consistent() const = 0;
  virtual bool is_local() const { return !is_consistent(); }
  virtual bool is_lazy() const = 0;
  virtual bool is_eager() const { return !is_lazy(); }
  virtual bool is_contiguous() const = 0;
  virtual Maybe<bool> is_pinned() const = 0;
  virtual const TensorMeta& tensor_meta() const = 0;
  virtual Maybe<Tensor> data() = 0;
  virtual std::shared_ptr<Tensor> pin_memory() const = 0;
  virtual Maybe<Symbol<ConsistentTensorMeta>> consistent_tensor_meta() const { OF_UNIMPLEMENTED(); }

  // Getters valid only for EagerMirroredTensor
  virtual Maybe<EagerMirroredTensorImpl*> mut_eager_mirrored_tensor_impl() { OF_UNIMPLEMENTED(); }
  virtual Maybe<vm::EagerBlobObject> eager_blob_object() const = 0;
  virtual Maybe<LocalDepObject*> compute_local_dep_object() const = 0;
  virtual Maybe<bool> has_eager_blob_object() const = 0;
  virtual Maybe<TensorStorage> tensor_storage() const { OF_UNIMPLEMENTED(); }
  virtual Maybe<const Stride> stride() const { OF_UNIMPLEMENTED(); }
  virtual Maybe<int64_t> storage_offset() const { OF_UNIMPLEMENTED(); }

  // Getters/Setters valid only for EagerConsistentTensor
  virtual Maybe<const Optional<Symbol<NdSbp>>&> consumer_nd_sbp_constraint() const {
    OF_UNIMPLEMENTED();
  }
  virtual Maybe<MirroredTensor> cur_rank_phy_tensor() const { OF_UNIMPLEMENTED(); }
  virtual Maybe<void> set_consumer_nd_sbp_constraint(const Optional<Symbol<NdSbp>>& val) {
    OF_UNIMPLEMENTED();
  }

  // Getters for autograd
  virtual bool requires_grad() const = 0;
  virtual bool is_leaf() const = 0;
  virtual bool retain_grad() const = 0;
  virtual std::shared_ptr<const FunctionNode> grad_fn_node() const = 0;
  virtual Maybe<Tensor> acc_grad() const = 0;
  virtual Maybe<TensorArg> current_grad() const = 0;
  virtual Maybe<Tensor> detach() const = 0;
  virtual Maybe<Tensor> clone() const = 0;
  virtual std::shared_ptr<Tensor> contiguous() const = 0;

  // Setters for autograd
  virtual Maybe<void> set_requires_grad(bool requires_grad) = 0;
  virtual Maybe<void> set_retain_grad(bool retain_grad) = 0;
  virtual void set_grad_fn_node(const std::shared_ptr<FunctionNode>& grad_fn_node) = 0;
  virtual std::shared_ptr<FunctionNode> mut_grad_fn_node() = 0;
  virtual Maybe<void> set_acc_grad(const std::shared_ptr<Tensor>& grad) = 0;
  virtual Maybe<Tensor> mut_acc_grad() = 0;
  virtual void set_is_leaf(bool is_leaf) = 0;
  virtual std::shared_ptr<const AutogradMeta> autograd_meta() const = 0;
  virtual std::shared_ptr<AutogradMeta> mut_autograd_meta() = 0;
  virtual void set_autograd_meta(const std::shared_ptr<AutogradMeta>& autograd_meta) = 0;

  virtual user_op::TensorDesc* mut_tensor_meta() = 0;
  virtual Maybe<void> set_data(const std::shared_ptr<Tensor>& other) = 0;

  virtual Maybe<void> RegisterStorageDeleteHook(const std::function<void()>& hook) {
    OF_UNIMPLEMENTED();
  };
  virtual Maybe<MirroredTensor> AsMirroredTensor() = 0;
  virtual Maybe<ConsistentTensor> AsConsistentTensor() = 0;

  // The same tensor instance should share the python object to ensure that
  // their id are consistent in Python. That is if x and y are hold the same tensor,
  // then `id(x)` should equal to `id(y)`
  void* pyobject() const { return pyobject_; }
  void set_pyobject(void* object) { pyobject_ = object; }

 protected:
  Tensor() : pyobject_(nullptr) {}

 private:
  void* pyobject_;
};

class StaticZerosTensor final : public Tensor {
 public:
  static Maybe<StaticZerosTensor> MakeTensor(const std::shared_ptr<const Shape>& shape,
                                             DataType dtype, Symbol<Device> device) {
    return std::shared_ptr<StaticZerosTensor>(new StaticZerosTensor(shape, dtype, device));
  }
  // Getters
  std::shared_ptr<const Shape> shape() const override { return shape_; }
  Symbol<DType> dtype() const override { return CHECK_JUST(DType::Get(dtype_)); }
  Maybe<TransportToken> transport_token() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<Symbol<NdSbp>> nd_sbp() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<Symbol<ParallelDesc>> parallel_desc() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<Symbol<Device>> device() const override { return device_; }
  Maybe<Symbol<Device>*> mut_device() override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  bool is_cuda() const override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return false;
  }
  bool is_consistent() const override { return false; }
  bool is_local() const override { return !is_consistent(); }
  bool is_lazy() const override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return false;
  }
  bool is_eager() const override { return !is_lazy(); }
  const TensorMeta& tensor_meta() const override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return *(TensorMeta*)nullptr;
  }
  Maybe<Tensor> data() override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  std::shared_ptr<Tensor> pin_memory() const override {
    return std::const_pointer_cast<Tensor>(shared_from_this());
  }
  Maybe<Symbol<ConsistentTensorMeta>> consistent_tensor_meta() const override {
    RETURN_ERROR_WITH_BUG_PROMPT();
  }

  // Getters valid only for EagerMirroredTensor
  Maybe<EagerMirroredTensorImpl*> mut_eager_mirrored_tensor_impl() override {
    RETURN_ERROR_WITH_BUG_PROMPT();
  }
  Maybe<vm::EagerBlobObject> eager_blob_object() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<LocalDepObject*> compute_local_dep_object() const override {
    RETURN_ERROR_WITH_BUG_PROMPT();
  }
  Maybe<bool> has_eager_blob_object() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<TensorStorage> tensor_storage() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<const Stride> stride() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<int64_t> storage_offset() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }

  // Getters/Setters valid only for EagerConsistentTensor
  Maybe<const Optional<Symbol<NdSbp>>&> consumer_nd_sbp_constraint() const override {
    RETURN_ERROR_WITH_BUG_PROMPT();
  }
  Maybe<MirroredTensor> cur_rank_phy_tensor() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<void> set_consumer_nd_sbp_constraint(const Optional<Symbol<NdSbp>>& val) override {
    RETURN_ERROR_WITH_BUG_PROMPT();
  }

  // Getters for autograd
  bool requires_grad() const override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return false;
  }
  bool is_leaf() const override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return false;
  }
  bool retain_grad() const override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return false;
  }
  bool is_contiguous() const override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return true;
  }
  Maybe<bool> is_pinned() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  std::shared_ptr<const FunctionNode> grad_fn_node() const override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return nullptr;
  }
  Maybe<Tensor> acc_grad() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<TensorArg> current_grad() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<Tensor> detach() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<Tensor> clone() const override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  std::shared_ptr<Tensor> contiguous() const override {
    return std::const_pointer_cast<Tensor>(shared_from_this());
  }

  // Setters for autograd
  Maybe<void> set_requires_grad(bool requires_grad) override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return Maybe<void>::Ok();
  }
  Maybe<void> set_retain_grad(bool retain_grad) override {
    RETURN_ERROR_WITH_BUG_PROMPT();
    return Maybe<void>::Ok();
  }
  void set_grad_fn_node(const std::shared_ptr<FunctionNode>& grad_fn_node) override {
    PRINT_BUG_PROMPT_AND_ABORT();
  }
  std::shared_ptr<FunctionNode> mut_grad_fn_node() override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return *(std::shared_ptr<FunctionNode>*)nullptr;
  }
  Maybe<void> set_acc_grad(const std::shared_ptr<Tensor>& grad) override {
    RETURN_ERROR_WITH_BUG_PROMPT();
  }
  Maybe<Tensor> mut_acc_grad() override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  void set_is_leaf(bool is_leaf) override { PRINT_BUG_PROMPT_AND_ABORT(); }
  std::shared_ptr<const AutogradMeta> autograd_meta() const override {
    PRINT_BUG_PROMPT_AND_ABORT();
  }
  std::shared_ptr<AutogradMeta> mut_autograd_meta() override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return nullptr;
  }
  void set_autograd_meta(const std::shared_ptr<AutogradMeta>& autograd_meta) override {
    PRINT_BUG_PROMPT_AND_ABORT();
  }

  user_op::TensorDesc* mut_tensor_meta() override {
    PRINT_BUG_PROMPT_AND_ABORT();
    return nullptr;
  }
  Maybe<void> set_data(const std::shared_ptr<Tensor>& other) override {
    RETURN_ERROR_WITH_BUG_PROMPT();
  }

  Maybe<MirroredTensor> AsMirroredTensor() override;
  Maybe<ConsistentTensor> AsConsistentTensor() override { RETURN_ERROR_WITH_BUG_PROMPT(); }

 private:
  StaticZerosTensor(const std::shared_ptr<const Shape>& shape, DataType dtype,
                    Symbol<Device> device)
      : shape_(shape), dtype_(dtype), device_(device) {}
  const std::shared_ptr<const Shape> shape_;
  DataType dtype_;
  Symbol<Device> device_;
};

template<typename DerivedT>
class TensorIf : public Tensor {
 public:
  virtual ~TensorIf() = default;

  // Getters for autograd
  // acc_grad is tensor's accumulated grad in more than once backward operation,
  // and current_grad is temporary grad to shared data with different FunctionNode
  std::shared_ptr<const FunctionNode> grad_fn_node() const override { return grad_fn_node_; }

  // Setters for autograd
  void set_grad_fn_node(const std::shared_ptr<FunctionNode>& grad_fn_node) override {
    grad_fn_node_ = grad_fn_node;
  }
  std::shared_ptr<FunctionNode> mut_grad_fn_node() override { return grad_fn_node_; }

 protected:
  TensorIf() = default;
  std::shared_ptr<FunctionNode> grad_fn_node_;
};

template<typename DerivedT>
class ProxyTensor : public TensorIf<DerivedT> {
 public:
  ProxyTensor(const std::shared_ptr<Tensor>& tensor) : tensor_(tensor) {}
  virtual ~ProxyTensor() = default;

  virtual std::shared_ptr<const Shape> shape() const override { return tensor_->shape(); }
  virtual Symbol<DType> dtype() const override { return tensor_->dtype(); }
  virtual Maybe<Symbol<NdSbp>> nd_sbp() const override { return tensor_->nd_sbp(); }
  virtual Maybe<Symbol<ParallelDesc>> parallel_desc() const override {
    return tensor_->parallel_desc();
  }
  virtual Maybe<Symbol<Device>> device() const override { return tensor_->device(); }
  virtual Maybe<Symbol<Device>*> mut_device() override { return tensor_->mut_device(); }
  virtual bool is_cuda() const override { return tensor_->is_cuda(); }
  virtual bool is_consistent() const override { return tensor_->is_consistent(); }
  virtual bool is_local() const override { return tensor_->is_local(); }
  virtual bool is_lazy() const override { return tensor_->is_lazy(); }
  virtual bool is_eager() const override { return tensor_->is_eager(); }
  virtual const TensorMeta& tensor_meta() const override { return tensor_->tensor_meta(); }
  virtual Maybe<Symbol<ConsistentTensorMeta>> consistent_tensor_meta() const override {
    return tensor_->consistent_tensor_meta();
  }
  virtual Maybe<Tensor> data() override { return tensor_->detach(); }
  virtual std::shared_ptr<Tensor> pin_memory() const override { return tensor_->pin_memory(); }

  // Must override grad_fn_node function. Otherwise grad_fn will belong to this not tensor_,
  // and it will be wrong when use Tensor.data() in operators.
  virtual std::shared_ptr<const FunctionNode> grad_fn_node() const override {
    return tensor_->grad_fn_node();
  }
  virtual void set_grad_fn_node(const std::shared_ptr<FunctionNode>& grad_fn_node) override {
    tensor_->set_grad_fn_node(grad_fn_node);
  }
  virtual std::shared_ptr<FunctionNode> mut_grad_fn_node() override {
    return tensor_->mut_grad_fn_node();
  }

  virtual Maybe<EagerMirroredTensorImpl*> mut_eager_mirrored_tensor_impl() override {
    return tensor_->mut_eager_mirrored_tensor_impl();
  }
  virtual Maybe<vm::EagerBlobObject> eager_blob_object() const override {
    return tensor_->eager_blob_object();
  }
  virtual Maybe<LocalDepObject*> compute_local_dep_object() const override {
    return tensor_->compute_local_dep_object();
  }
  virtual Maybe<bool> has_eager_blob_object() const override {
    return tensor_->has_eager_blob_object();
  }
  virtual Maybe<TensorStorage> tensor_storage() const override { return tensor_->tensor_storage(); }
  virtual Maybe<const Stride> stride() const override { return tensor_->stride(); }
  virtual Maybe<int64_t> storage_offset() const override { return tensor_->storage_offset(); }

  virtual Maybe<const Optional<Symbol<NdSbp>>&> consumer_nd_sbp_constraint() const override {
    return tensor_->consumer_nd_sbp_constraint();
  }
  virtual Maybe<TransportToken> transport_token() const override {
    return tensor_->transport_token();
  }
  virtual Maybe<MirroredTensor> cur_rank_phy_tensor() const override {
    return tensor_->cur_rank_phy_tensor();
  }
  virtual Maybe<void> set_consumer_nd_sbp_constraint(const Optional<Symbol<NdSbp>>& val) override {
    return tensor_->set_consumer_nd_sbp_constraint(val);
  }

  virtual bool requires_grad() const override { return tensor_->requires_grad(); }
  virtual bool is_leaf() const override { return tensor_->is_leaf(); }
  virtual bool retain_grad() const override { return tensor_->retain_grad(); }
  virtual bool is_contiguous() const override { return tensor_->is_contiguous(); }
  virtual Maybe<bool> is_pinned() const override { return tensor_->is_pinned(); }
  virtual Maybe<Tensor> acc_grad() const override { return tensor_->acc_grad(); }
  virtual Maybe<TensorArg> current_grad() const override { return tensor_->current_grad(); }
  virtual Maybe<Tensor> detach() const override { return tensor_->detach(); }
  virtual Maybe<Tensor> clone() const override { return tensor_->clone(); }

  virtual Maybe<void> set_requires_grad(bool requires_grad) override {
    return tensor_->set_requires_grad(requires_grad);
  }
  virtual Maybe<void> set_retain_grad(bool retain_grad) override {
    return tensor_->set_retain_grad(retain_grad);
  }
  virtual Maybe<void> set_acc_grad(const std::shared_ptr<Tensor>& grad) override {
    return tensor_->set_acc_grad(grad);
  }
  virtual Maybe<Tensor> mut_acc_grad() override { return tensor_->mut_acc_grad(); }
  virtual void set_is_leaf(bool is_leaf) override { return tensor_->set_is_leaf(is_leaf); }
  virtual std::shared_ptr<const AutogradMeta> autograd_meta() const override {
    return tensor_->autograd_meta();
  }
  virtual std::shared_ptr<AutogradMeta> mut_autograd_meta() override {
    return tensor_->mut_autograd_meta();
  }
  virtual void set_autograd_meta(const std::shared_ptr<AutogradMeta>& autograd_meta) override {
    return tensor_->set_autograd_meta(autograd_meta);
  }

  virtual user_op::TensorDesc* mut_tensor_meta() override { return tensor_->mut_tensor_meta(); }
  virtual Maybe<void> set_data(const std::shared_ptr<Tensor>& other) override {
    CHECK_OR_RETURN(is_local() == other->is_local() && is_eager() == other->is_eager())
        << "You can't assign copy between tensors with different type";
    bool old_requires_grad = tensor_->requires_grad();
    this->tensor_ = JUST(other->detach());
    JUST(this->tensor_->set_requires_grad(old_requires_grad));
    return Maybe<void>::Ok();
  }

  virtual Maybe<MirroredTensor> AsMirroredTensor() override {
    if (const auto& mirrored_tensor = std::dynamic_pointer_cast<MirroredTensor>(tensor_)) {
      return mirrored_tensor;
    }
    RETURN_ERROR_WITH_BUG_PROMPT();
  }

  virtual Maybe<ConsistentTensor> AsConsistentTensor() override {
    if (const auto& consistent_tensor = std::dynamic_pointer_cast<ConsistentTensor>(tensor_)) {
      return consistent_tensor;
    }
    RETURN_ERROR_WITH_BUG_PROMPT();
  }

 protected:
  std::shared_ptr<Tensor> tensor_;
};

class Parameter final : public ProxyTensor<Parameter> {
 public:
  static Maybe<Parameter> MakeTensor(const std::shared_ptr<Tensor>& tensor, bool requires_grad) {
    return std::shared_ptr<Parameter>(new Parameter(JUST(tensor->detach()), requires_grad));
  }
  bool is_leaf() const override { return true; }
  std::shared_ptr<Tensor> contiguous() const override;
  std::shared_ptr<Tensor> pin_memory() const override;

 private:
  Parameter(const std::shared_ptr<Tensor>& tensor, bool requires_grad)
      : ProxyTensor<Parameter>(tensor) {
    this->tensor_->set_requires_grad(requires_grad);
  }
};

class MirroredTensor final : public TensorIf<MirroredTensor> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(MirroredTensor);
  MirroredTensor() = default;
  explicit MirroredTensor(const std::shared_ptr<MirroredTensorImpl>& impl) { impl_ = impl; }
  ~MirroredTensor() override = default;

  // Getters
  std::shared_ptr<const Shape> shape() const override { return impl_->shape(); }
  Symbol<DType> dtype() const override { return CHECK_JUST(DType::Get(impl_->dtype())); }
  Maybe<TransportToken> transport_token() const override {
    OF_RUNTIME_ERROR() << "Only global tensors have 'global_id', global id is used to "
                          "synchronize rank";
  }
  Maybe<Symbol<NdSbp>> nd_sbp() const override {
    OF_RUNTIME_ERROR()
        << "Local tensor has no sbp property. "
           "sbp is the description in the oneflow distributed case, you can refer to "
           "https://docs.oneflow.org/master/parallelism/03_consistent_tensor.html; "
           "For example, create a global tensor like this : 'x = oneflow.tensor((2,3, "
           "placement=oneflow.placement(\"cuda\", {0: 0}), sbp=oneflow.sbp.broadcast))', then "
           "'x.sbp' is 'oneflow.sbp.broadcast'";
  }
  Maybe<Symbol<ParallelDesc>> parallel_desc() const override {
    OF_RUNTIME_ERROR() << "Only global tensors have 'placement'. Placement is used to describe "
                          "the distribution of global tensor in multiple GPUs. Please use "
                          "'.device' for local tensors.";
  }
  Maybe<Symbol<Device>> device() const override { return impl_->device(); }
  Maybe<Symbol<Device>*> mut_device() override { return impl_->mut_device(); }
  bool is_lazy() const override { return impl_->is_lazy(); }
  bool is_consistent() const override { return false; }
  bool is_cuda() const override;
  std::shared_ptr<Tensor> contiguous() const override;

  const TensorMeta& tensor_meta() const override { return *impl_->tensor_meta(); }
  Maybe<Tensor> data() override { return this->detach(); }
  std::shared_ptr<Tensor> pin_memory() const override;

  // Getters valid only for EagerMirroredTensor
  Maybe<vm::EagerBlobObject> eager_blob_object() const override {
    return impl_->eager_blob_object();
  }
  Maybe<LocalDepObject*> compute_local_dep_object() const override {
    return impl_->compute_local_dep_object();
  }
  Maybe<TensorStorage> tensor_storage() const override { return impl_->tensor_storage(); }
  Maybe<bool> has_eager_blob_object() const override { return impl_->has_eager_blob_object(); }
  Maybe<const Stride> stride() const override { return impl_->stride(); }
  Maybe<int64_t> storage_offset() const override { return impl_->storage_offset(); }

  // Getters for autograd
  Maybe<Tensor> acc_grad() const override { return impl_->acc_grad(); }
  Maybe<TensorArg> current_grad() const override { return impl_->current_grad(); }
  bool requires_grad() const override { return impl_->requires_grad(); }
  bool is_leaf() const override { return impl_->is_leaf(); }
  bool retain_grad() const override { return impl_->retain_grad(); }
  bool is_contiguous() const override { return impl_->is_contiguous(); }
  Maybe<bool> is_pinned() const override { return impl_->is_pinned(); };

  // Setters for autograd
  Maybe<void> set_acc_grad(const std::shared_ptr<Tensor>& grad) override {
    return impl_->set_acc_grad(grad);
  }
  Maybe<void> set_requires_grad(bool requires_grad) override {
    JUST(impl_->set_requires_grad(requires_grad));
    if (!requires_grad) { set_grad_fn_node(nullptr); }
    return Maybe<void>::Ok();
  }
  Maybe<void> set_retain_grad(bool retain_grad) override {
    return impl_->set_retain_grad(retain_grad);
  }
  Maybe<Tensor> mut_acc_grad() override { return impl_->mut_acc_grad(); }
  void set_is_leaf(bool is_leaf) override { impl_->set_is_leaf(is_leaf); }
  std::shared_ptr<const AutogradMeta> autograd_meta() const override {
    return impl_->autograd_meta();
  }
  std::shared_ptr<AutogradMeta> mut_autograd_meta() override { return impl_->mut_autograd_meta(); }
  void set_autograd_meta(const std::shared_ptr<AutogradMeta>& autograd_meta) override {
    impl_->set_autograd_meta(autograd_meta);
  }

  // Operators for tensor
  Maybe<Tensor> detach() const override;
  Maybe<Tensor> clone() const override;

  static Maybe<MirroredTensor> MakeTensor(const std::shared_ptr<const Shape>& shape,
                                          const std::shared_ptr<const Stride>& stride,
                                          DataType dtype, const Symbol<Device>& device,
                                          bool is_lazy, bool requires_grad, bool is_leaf);
  MirroredTensorImpl* mut_impl() { return impl_.get(); }
  Maybe<EagerMirroredTensorImpl*> mut_eager_mirrored_tensor_impl() override {
    return impl_->mut_eager_mirrored_tensor_impl();
  }
  user_op::TensorDesc* mut_tensor_meta() override { return impl_->mut_tensor_meta(); }
  Maybe<void> set_data(const std::shared_ptr<Tensor>& other) override {
    CHECK_OR_RETURN(this->is_leaf()) << "Can only set leaf tensor's data.";
    const auto& mirrored_tensor = std::dynamic_pointer_cast<MirroredTensor>(JUST(other->detach()));
    CHECK_NOTNULL_OR_RETURN(mirrored_tensor);
    bool old_requires_grad = requires_grad();
    impl_ = mirrored_tensor->impl_;
    set_requires_grad(old_requires_grad);
    grad_fn_node_ = nullptr;
    return Maybe<void>::Ok();
  }

  Maybe<void> RegisterStorageDeleteHook(const std::function<void()>& hook) override {
    return impl_->RegisterStorageDeleteHook(hook);
  }

  Maybe<MirroredTensor> AsMirroredTensor() override {
    return std::dynamic_pointer_cast<MirroredTensor>(shared_from_this());
  }
  Maybe<ConsistentTensor> AsConsistentTensor() override { RETURN_ERROR_WITH_BUG_PROMPT(); }

 private:
  std::shared_ptr<MirroredTensorImpl> impl_;
};

class ConsistentTensor final : public TensorIf<ConsistentTensor> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(ConsistentTensor);
  ConsistentTensor() = default;
  explicit ConsistentTensor(const std::shared_ptr<ConsistentTensorImpl>& impl) { impl_ = impl; }
  ~ConsistentTensor() override = default;

  // Getters
  std::shared_ptr<const Shape> shape() const override { return impl_->shape(); }
  Symbol<DType> dtype() const override { return CHECK_JUST(DType::Get(impl_->dtype())); }
  Maybe<TransportToken> transport_token() const override { return impl_->transport_token(); }
  Maybe<Symbol<NdSbp>> nd_sbp() const override { return impl_->nd_sbp(); }
  Maybe<Symbol<ParallelDesc>> parallel_desc() const override { return impl_->parallel_desc(); }
  Maybe<Symbol<Device>> device() const override {
    OF_RUNTIME_ERROR() << "Only local tensors have 'device'. Please use "
                          "'.placement' for global tensors.";
  }
  Maybe<Symbol<Device>*> mut_device() override {
    OF_RUNTIME_ERROR() << "ConsistentTensor has no mut_device property";
  }
  bool is_lazy() const override { return impl_->is_lazy(); }
  bool is_consistent() const override { return true; }
  Maybe<const Optional<Symbol<NdSbp>>&> consumer_nd_sbp_constraint() const override {
    return impl_->consumer_nd_sbp_constraint();
  }
  Maybe<MirroredTensor> cur_rank_phy_tensor() const override {
    return impl_->cur_rank_phy_tensor();
  }
  bool is_cuda() const override;
  std::shared_ptr<Tensor> contiguous() const override;
  Maybe<Tensor> data() override { return this->detach(); }
  Maybe<const Stride> stride() const override { return impl_->stride(); }
  std::shared_ptr<Tensor> pin_memory() const override;

  // Getters valid only for EagerMirroredTensor
  Maybe<vm::EagerBlobObject> eager_blob_object() const override {
    return impl_->eager_blob_object();
  }
  Maybe<LocalDepObject*> compute_local_dep_object() const override {
    return impl_->compute_local_dep_object();
  }
  const TensorMeta& tensor_meta() const override { return *impl_->tensor_meta(); }
  Maybe<TensorStorage> tensor_storage() const override { return impl_->tensor_storage(); }
  Maybe<bool> has_eager_blob_object() const override { return impl_->has_eager_blob_object(); }

  // Setters
  Maybe<void> set_consumer_nd_sbp_constraint(const Optional<Symbol<NdSbp>>& val) override {
    impl_->set_consumer_nd_sbp_constraint(val);
    return Maybe<void>::Ok();
  }

  // Getters for autograd
  Maybe<Tensor> acc_grad() const override { return impl_->acc_grad(); }
  Maybe<TensorArg> current_grad() const override { return impl_->current_grad(); }
  bool requires_grad() const override { return impl_->requires_grad(); }
  bool is_leaf() const override { return impl_->is_leaf(); }
  bool retain_grad() const override { return impl_->retain_grad(); }
  bool is_contiguous() const override { return impl_->is_contiguous(); }
  Maybe<bool> is_pinned() const override {
    OF_RUNTIME_ERROR() << "ConsistentTensor has no is_pinned method";
  }

  // Setters for autograd
  Maybe<void> set_acc_grad(const std::shared_ptr<Tensor>& grad) override {
    return impl_->set_acc_grad(grad);
  }
  Maybe<Tensor> mut_acc_grad() override { return impl_->mut_acc_grad(); }
  Maybe<void> set_requires_grad(bool requires_grad) override {
    JUST(impl_->set_requires_grad(requires_grad));
    if (!requires_grad) { set_grad_fn_node(nullptr); }
    return Maybe<void>::Ok();
  }
  Maybe<void> set_retain_grad(bool retain_grad) override {
    return impl_->set_retain_grad(retain_grad);
  }
  void set_is_leaf(bool is_leaf) override { impl_->set_is_leaf(is_leaf); }
  std::shared_ptr<const AutogradMeta> autograd_meta() const override {
    return impl_->autograd_meta();
  }
  std::shared_ptr<AutogradMeta> mut_autograd_meta() override { return impl_->mut_autograd_meta(); }
  void set_autograd_meta(const std::shared_ptr<AutogradMeta>& autograd_meta) override {
    impl_->set_autograd_meta(autograd_meta);
  }

  // Operators for tensor
  Maybe<Tensor> detach() const override;
  Maybe<Tensor> clone() const override;

  static Maybe<ConsistentTensor> MakeTensor(const std::shared_ptr<const Shape>& shape,
                                            DataType dtype, Symbol<NdSbp> nd_sbp,
                                            Symbol<ParallelDesc> parallel_desc, bool is_lazy,
                                            bool requires_grad, bool is_leaf);

  ConsistentTensorImpl* mut_impl() { return impl_.get(); }

  Maybe<Symbol<ConsistentTensorMeta>> consistent_tensor_meta() const override {
    return impl_->tensor_meta();
  }

  user_op::TensorDesc* mut_tensor_meta() override { return impl_->mut_tensor_meta(); }
  Maybe<void> set_data(const std::shared_ptr<Tensor>& other) override;

  Maybe<MirroredTensor> AsMirroredTensor() override { RETURN_ERROR_WITH_BUG_PROMPT(); }
  Maybe<ConsistentTensor> AsConsistentTensor() override {
    return std::dynamic_pointer_cast<ConsistentTensor>(shared_from_this());
  }

 private:
  std::shared_ptr<ConsistentTensorImpl> impl_;
};

}  // namespace one
}  // namespace oneflow

#endif  // ONEFLOW_CORE_FRAMEWORK_TENSOR_H_
