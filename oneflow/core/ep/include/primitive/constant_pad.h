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
#ifndef ONEFLOW_CORE_EP_PRIMITIVE_CONSTANT_PAD_H_
#define ONEFLOW_CORE_EP_PRIMITIVE_CONSTANT_PAD_H_

#include "oneflow/core/ep/include/primitive/primitive.h"
#include "oneflow/core/common/scalar.h"

namespace oneflow {

namespace ep {

namespace primitive {

class ConstantPad : public Primitive {
 public:
  OF_DISALLOW_COPY_AND_MOVE(ConstantPad);
  ConstantPad() = default;
  ~ConstantPad() override = default;

  virtual void Launch(Stream* stream, size_t num_dims, const int64_t* src_dims, const void* src,
                      const int64_t* padding_before, const int64_t* padding_after, Scalar pad_val,
                      void* dst) = 0;
};

class ConstantPadFactory : public Factory<ConstantPad> {
 public:
  OF_DISALLOW_COPY_AND_MOVE(ConstantPadFactory);
  ConstantPadFactory() = default;
  ~ConstantPadFactory() override = default;

  virtual std::unique_ptr<ConstantPad> New(DataType data_type) = 0;
};

}  // namespace primitive

}  // namespace ep

}  // namespace oneflow

#endif
