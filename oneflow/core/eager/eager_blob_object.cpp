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
#include "oneflow/core/eager/eager_blob_object.h"
#include "oneflow/core/vm/allocator.h"
#include "oneflow/core/framework/to_string.h"
#include "oneflow/core/framework/shut_down_util.h"
#include "oneflow/core/common/shape_vec.h"

namespace oneflow {
namespace vm {

EagerBlobObject::EagerBlobObject(const std::shared_ptr<MemoryCase>& mem_case,
                                 const std::shared_ptr<Shape>& shape, DataType data_type,
                                 const std::shared_ptr<TensorStorage>& tensor_storage,
                                 const intrusive::shared_ptr<LocalDepObject>& dep_object)
    : is_dynamic_(false),
      mem_case_(mem_case),
      data_type_(data_type),
      shape_view_(*shape),
      mut_shape_view_(const_cast<int64_t*>(shape->dim_vec().data()), shape->dim_vec().size()),
      shape_(shape),
      storage_offset_(0),
      tensor_storage_(tensor_storage),
      is_shape_synced_(true),
      compute_local_dep_object_(dep_object) {
  CHECK(static_cast<bool>(shape));
  CHECK(static_cast<bool>(tensor_storage));
}

std::shared_ptr<Blob> EagerBlobObject::AsBlob(const BlobDesc* blob_desc) {
  return std::shared_ptr<Blob>(new Blob(*mem_case_, blob_desc,
                                        reinterpret_cast<char*>(mut_shape_view_.mut_ptr()),
                                        mut_dptr<char>()));
}

void EagerBlobObject::set_storage_offset(const int64_t offset) {
  storage_offset_ = offset;
}

Maybe<void> EagerBlobObject::TryAllocateBlobBodyMemory(DeviceCtx* device_ctx) {
  vm::Allocator* allocator = device_ctx->mut_allocator();
  CHECK_NOTNULL_OR_RETURN(allocator);
  size_t required_body_bytes = GetBlobBodyAlignedSize(ByteSizeOfBlobBody());
  if (required_body_bytes == 0) {
    CHECK_ISNULL_OR_RETURN(tensor_storage_->blob_dptr());
    return Maybe<void>::Ok();
  }
  if (tensor_storage_->blob_dptr() != nullptr) {
    CHECK_GE_OR_RETURN(tensor_storage_->blob_bytes(), required_body_bytes);
    return Maybe<void>::Ok();
  }
  {
    // reset tensor_storage_;
    const auto& Free = [allocator, required_body_bytes](char* dptr) {
      if (IsShuttingDown()) { return; }
      allocator->Deallocate(dptr, required_body_bytes);
    };
    char* dptr = nullptr;
    allocator->Allocate(&dptr, required_body_bytes);
    tensor_storage_->set_blob_dptr(std::unique_ptr<char, std::function<void(char*)>>(dptr, Free),
                                   required_body_bytes);

    // blob->reset_dptr(dptr);
    InitNonPODTypeEagerBlobObjectIfNeed(tensor_storage_->non_pod_allocator(), this);
  }
  return Maybe<void>::Ok();
}

}  // namespace vm
}  // namespace oneflow
