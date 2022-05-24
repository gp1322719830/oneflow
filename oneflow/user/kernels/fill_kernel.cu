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
#include "oneflow/core/framework/framework.h"
#include "oneflow/core/kernel/new_kernel_util.h"
#include "oneflow/core/common/nd_index_offset_helper.h"

namespace oneflow {

namespace {
template<typename T>
__global__ void FillForwardGpu(const int n, const T value, T* y) {
  CUDA_1D_KERNEL_LOOP(i, n) { y[i] = value; }
}

template<typename T>
__global__ void FillGradForwardGpu(const int n, T* y) {
  CUDA_1D_KERNEL_LOOP(i, n) { y[i] = 0; }
}

template<typename T>
T GetDtypeMatchedValue(double floating, int64_t integral);

template<>
float GetDtypeMatchedValue(double floating, int64_t integral) {
  return static_cast<float>(floating);
}

template<>
double GetDtypeMatchedValue(double floating, int64_t integral) {
  return floating;
}

template<>
int8_t GetDtypeMatchedValue(double floating, int64_t integral) {
  return static_cast<int8_t>(integral);
}

template<>
int32_t GetDtypeMatchedValue(double floating, int64_t integral) {
  return static_cast<int32_t>(integral);
}

template<>
int64_t GetDtypeMatchedValue(double floating, int64_t integral) {
  return integral;
}

};  // namespace

template<typename T>
class FillGpuKernel final : public user_op::OpKernel {
 public:
  FillGpuKernel() = default;
  ~FillGpuKernel() = default;

 private:
  using user_op::OpKernel::Compute;
  void Compute(user_op::KernelComputeContext* ctx) const override {
    const user_op::Tensor* x = ctx->Tensor4ArgNameAndIndex("x", 0);
    user_op::Tensor* y = ctx->Tensor4ArgNameAndIndex("y", 0);
    double floating_value = ctx->Attr<double>("floating_value");
    int64_t integral_value = ctx->Attr<int64_t>("integral_value");
    const T value_ = GetDtypeMatchedValue<T>(floating_value, integral_value);
    const int32_t elem_cnt = x->shape().elem_cnt();
    T* y_ptr = y->mut_dptr<T>();
    RUN_CUDA_KERNEL((FillForwardGpu<T>), ctx->stream(), elem_cnt, elem_cnt, value_, y_ptr);
  }
  bool AlwaysComputeWhenAllOutputsEmpty() const override { return false; }
};

template<typename T>
class FillGradGpuKernel final : public user_op::OpKernel {
 public:
  FillGradGpuKernel() = default;
  ~FillGradGpuKernel() = default;

 private:
  void Compute(user_op::KernelComputeContext* ctx) const override {
    const user_op::Tensor* in = ctx->Tensor4ArgNameAndIndex("in", 0);
    user_op::Tensor* out = ctx->Tensor4ArgNameAndIndex("out", 0);
    const int32_t elem_cnt = in->shape().elem_cnt();
    T* out_ptr = out->mut_dptr<T>();
    RUN_CUDA_KERNEL((FillGradForwardGpu<T>), ctx->stream(), elem_cnt, elem_cnt, out_ptr);
  }
  bool AlwaysComputeWhenAllOutputsEmpty() const override { return false; }
};

#define REGISTER_FILL_CUDA_KERNEL(dtype)                                             \
  REGISTER_USER_KERNEL("fill_").SetCreateFn<FillGpuKernel<dtype>>().SetIsMatchedHob( \
      (user_op::HobDeviceType() == DeviceType::kCUDA)                                \
      && (user_op::HobDataType("y", 0) == GetDataType<dtype>::value));

#define REGISTER_FILL_Grad_CUDA_KERNEL(dtype)                          \
  REGISTER_USER_KERNEL("fill_grad")                                    \
      .SetCreateFn<FillGradGpuKernel<dtype>>()                         \
      .SetIsMatchedHob((user_op::HobDeviceType() == DeviceType::kCUDA) \
                       && (user_op::HobDataType("in", 0) == GetDataType<dtype>::value));

REGISTER_FILL_CUDA_KERNEL(float);
REGISTER_FILL_CUDA_KERNEL(double);
REGISTER_FILL_CUDA_KERNEL(int8_t);
REGISTER_FILL_CUDA_KERNEL(int32_t);
REGISTER_FILL_CUDA_KERNEL(int64_t);
REGISTER_FILL_Grad_CUDA_KERNEL(float);
REGISTER_FILL_Grad_CUDA_KERNEL(double);
REGISTER_FILL_Grad_CUDA_KERNEL(int8_t);
REGISTER_FILL_Grad_CUDA_KERNEL(int32_t);
REGISTER_FILL_Grad_CUDA_KERNEL(int64_t);

}  // namespace oneflow