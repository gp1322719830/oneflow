#include "oneflow/core/operator/operator.h"
#include "oneflow/core/graph/logical_node.h"
#include "oneflow/core/common/balanced_splitter.h"

namespace oneflow {

namespace {

DataType GetDataTypeFromBnInOpVec(
    std::function<const BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const PbRpf<std::string>& bn_in_ops) {
  for (const std::string& bn_in_op : bn_in_ops) {
    const BlobDesc* blob_desc = GetBlobDesc4BnInOp(bn_in_op);
    if (blob_desc) { return blob_desc->data_type(); }
  }
  return DataType::kInvalidDataType;
}

}  // namespace

void Operator::InitFromOpConf(const OperatorConf& op_conf) {
  OperatorConf* this_op_conf = op_attribute_.mutable_op_conf();
  *this_op_conf = op_conf;
  if (Global<JobDesc>::Get()->IsPredict()) { this_op_conf->set_trainable(false); }
  if (this_op_conf->has_enable_cudnn() == false) {
    this_op_conf->set_enable_cudnn(Global<JobDesc>::Get()->EnableCudnn());
  }
  if (GetActivationType() != ActivationType::kNone) { EnrollBwBufBn("bw_activation"); }
  InitFromOpConf();
}

LogicalNode* Operator::NewProperLogicalNode() { return new NormalForwardLogicalNode; }

const LogicalBlobId& Operator::BnInOp2Lbi(const std::string& bn_in_op) const {
  return op_attribute_.bn_in_op2lbi().at(bn_in_op);
}

LogicalBlobId* Operator::MutBnInOp2Lbi(const std::string& bn_in_op) {
  auto it = op_attribute_.mutable_bn_in_op2lbi()->find(bn_in_op);
  if (it == op_attribute_.mutable_bn_in_op2lbi()->end()) {
    return nullptr;
  } else {
    return &(it->second);
  }
}

const std::string& Operator::SoleIbn() const {
  CHECK_EQ(input_bns().size(), 1);
  return input_bns().Get(0);
}
const std::string& Operator::SoleIdbn() const {
  CHECK_EQ(input_diff_bns().size(), 1);
  return input_diff_bns().Get(0);
}
const std::string& Operator::SoleObn() const {
  CHECK_EQ(output_bns().size(), 1);
  return output_bns().Get(0);
}
const std::string& Operator::SoleOdbn() const {
  CHECK_EQ(output_diff_bns().size(), 1);
  return output_diff_bns().Get(0);
}
const std::string& Operator::SoleDtbn() const {
  CHECK_EQ(data_tmp_bns().size(), 1);
  return data_tmp_bns().Get(0);
}
const std::string& Operator::SoleFbbn() const {
  CHECK_EQ(fw_buf_bns().size(), 1);
  return fw_buf_bns().Get(0);
}
const std::string& Operator::SoleBbbn() const {
  CHECK_EQ(bw_buf_bns().size(), 1);
  return bw_buf_bns().Get(0);
}

void Operator::InferBlobDescsIf(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                                const ParallelContext* parallel_ctx, int64_t record_piece_size,
                                std::function<void(OpContext*)> EnrollOpCtx) const {
  InferBlobDescs(GetBlobDesc4BnInOp, parallel_ctx, record_piece_size, EnrollOpCtx);
  if (op_attribute_.model_bns().size() > 0) {
    InferTotalInstanceNumDesc(GetBlobDesc4BnInOp, parallel_ctx, EnrollOpCtx);
  }
}

void Operator::InferBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                              const ParallelContext* parallel_ctx, int64_t record_piece_size,
                              std::function<void(OpContext*)> EnrollOpCtx) const {
  InferBlobDescs(GetBlobDesc4BnInOp, parallel_ctx, record_piece_size);
}

void Operator::InferBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                              const ParallelContext* parallel_ctx,
                              int64_t record_piece_size) const {
  InferBlobDescs(GetBlobDesc4BnInOp, parallel_ctx);
}

void Operator::InferBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                              const ParallelContext* parallel_ctx) const {
  UNIMPLEMENTED() << typeid(*this).name();
}

void Operator::InferBwBufBlobDescsIf(
    std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const ParallelContext* parallel_ctx, const OpContext* op_ctx) const {
  InferBwBufBlobDescs(GetBlobDesc4BnInOp, parallel_ctx, op_ctx);
  if (GetActivationType() != ActivationType::kNone) {
    *GetBlobDesc4BnInOp("bw_activation") = *GetBlobDesc4BnInOp(SoleOdbn());
  }
}

void Operator::InferOutputBlobTimeShapeIf(
    std::function<const Shape*(const std::string&)> GetTimeShape4BnInOp,
    const ParallelContext* parallel_ctx, Shape* time_shape) const {
  for (const std::string& ibn : input_bns()) {
    CHECK_EQ(*GetTimeShape4BnInOp(ibn), *GetTimeShape4BnInOp(input_bns().Get(0)));
  }
  InferOutputBlobTimeShape(GetTimeShape4BnInOp, parallel_ctx, time_shape);
}

void Operator::InferOutputBlobTimeShape(
    std::function<const Shape*(const std::string&)> GetTimeShape4BnInOp, const ParallelContext*,
    Shape* time_shape) const {
  for (const std::string& bn : input_bns()) {
    CHECK_EQ(*GetTimeShape4BnInOp(input_bns().Get(0)), *GetTimeShape4BnInOp(bn));
  }
  if (input_bns().empty() == false) {
    *time_shape = *GetTimeShape4BnInOp(input_bns().Get(0));
  } else {
    *time_shape = Shape(
        {Global<JobDesc>::Get()->TotalBatchNum(), Global<JobDesc>::Get()->NumOfPiecesInBatch()});
  }
}

int32_t Operator::OutputBlobModelSplitAxis(
    const std::function<const SbpInferHint&(const std::string&)>& SbpInferHint4Ibn,
    const std::string& obn) const {
  if (IsSoleInputBlobAllowedModelSplit()) {
    return SbpInferHint4Ibn(SoleIbn()).split_axis();
  } else {
    UNIMPLEMENTED();
    return -1;
  }
}

void Operator::GetOpParallelSignatures(
    std::vector<std::unique_ptr<const OpParallelSignature>>* op_parallel_signatures) const {
  bool has_model = !(model_bns().empty() && const_model_bns().empty());
  op_parallel_signatures->emplace_back(MakeDataSplitOpParallelSignature(this));
  if (IsSoleInputBlobAllowedModelSplit()) {
    CHECK(!has_model);
    op_parallel_signatures->emplace_back(MakeModelSplitOpParallelSignature(this));
    op_parallel_signatures->emplace_back(MakeBroadcastOpParallelSignature(this));
  } else if (has_model) {
    for (const auto& ibn : input_bns()) { CHECK(!IsInputBlobAllowedModelSplit(ibn)); }
    op_parallel_signatures->emplace_back(MakeModelSplitOpParallelSignature(this));
  } else if (input_bns().size() == 1) {
    op_parallel_signatures->emplace_back(MakeBroadcastOpParallelSignature(this));
  } else {
    // do nothing
  }
}

void Operator::InferInputOutputSbpParallelIf(
    std::function<SbpParallel*(const std::string&)> SbpParallel4BnInOp,
    std::function<const SbpInferHint&(const std::string&)> SbpInferHint4Ibn,
    const ParallelDesc& parallel_desc) const {
  std::vector<std::unique_ptr<const OpParallelSignature>> op_parallel_signatures;
  GetOpParallelSignatures(&op_parallel_signatures);
  std::vector<OpParallelMatchResult> match_results;
  for (const auto& signature : op_parallel_signatures) {
    match_results.push_back(signature->GetMatchResult(SbpInferHint4Ibn, parallel_desc));
  }
  int32_t match_success_cnt = 0;
  for (const auto& result : match_results) {
    if (result.has_success()) { ++match_success_cnt; }
  }
  if (match_success_cnt == 1) {
    const OpParallelSignature* match_signature = nullptr;
    FOR_RANGE(int32_t, i, 0, op_parallel_signatures.size()) {
      if (match_results.at(i).has_success()) {
        match_signature = op_parallel_signatures.at(i).get();
      }
    }
    HashMap<std::string, SbpParallel> bn2sbp;
    match_signature->GenerateSignature(SbpInferHint4Ibn, &bn2sbp);
    for (const auto& pair : bn2sbp) {
      auto* sbp_parallel = SbpParallel4BnInOp(pair.first);
      *sbp_parallel = pair.second;
    }
  } else if (match_success_cnt == 0) {
    std::stringstream ss;
    FOR_RANGE(int32_t, i, 0, op_parallel_signatures.size()) {
      CHECK(match_results.at(i).has_fail());
      const auto& failed_msg = match_results.at(i).fail();
      ss << "op_parallel_signature match failed\n"
         << op_parallel_signatures.at(i)->Description() << ":\n";
      if (failed_msg.has_signature_mismatch()) {
        ss << "\t"
           << "signature mismatch"
           << "\n";
      } else {
        CHECK(failed_msg.has_conf_error());
        if (failed_msg.conf_error().has_parallel_policy_error()) {
          const auto& policy_error_msg = failed_msg.conf_error().parallel_policy_error();
          ss << "\t"
             << "parallel_policy conf error, configured: "
             << ParallelPolicy_Name(policy_error_msg.configured())
             << ", expected: " << ParallelPolicy_Name(policy_error_msg.expected()) << "\n";
        }
        if (failed_msg.conf_error().has_parallel_num_error()) {
          const auto& parallel_num_error_msg = failed_msg.conf_error().parallel_num_error();
          ss << "\t"
             << "parallel_num conf error, configured: " << parallel_num_error_msg.configured()
             << ", expected: " << parallel_num_error_msg.expected() << "\n";
        }
        if (failed_msg.conf_error().has_device_set_error()) {
          const auto& device_set_error_msg = failed_msg.conf_error().device_set_error();
          ss << "\t"
             << "device_set conf error, configured: " << device_set_error_msg.configured()
             << ", expected: " << device_set_error_msg.expected() << "\n";
        }
      }
    }
    LOG(FATAL) << ss.str();
  } else {
    UNIMPLEMENTED();
  }
}

bool Operator::IsSoleInputBlobAllowedModelSplit() const {
  return input_bns().size() == 1 && IsInputBlobAllowedModelSplit(SoleIbn());
}

void Operator::InferIsModelBlob4OutputBlobsIf(
    std::function<bool*(const std::string&)> IsModelBlob4BnInOp) const {
  InferIsModelBlob4OutputBlobs(IsModelBlob4BnInOp);
}

void Operator::InferIsModelBlob4OutputBlobs(
    std::function<bool*(const std::string&)> IsModelBlob4BnInOp) const {
  bool is_model_blob = (IsSoleInputBlobAllowedModelSplit() && *IsModelBlob4BnInOp(SoleIbn()));
  for (const std::string& obn : output_bns()) { *IsModelBlob4BnInOp(obn) = is_model_blob; }
}

void Operator::InferBwBufBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                                   const ParallelContext* parallel_ctx,
                                   const OpContext* op_ctx) const {
  InferBwBufBlobDescs(GetBlobDesc4BnInOp, parallel_ctx);
}

void Operator::FixInDiffBlobDescs(std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                                  const ParallelContext* ctx) const {
  VirtualFixInDiffBlobDescs(GetBlobDesc4BnInOp, ctx);
  for (const std::string& input_diff_bn : input_diff_bns()) {
    BlobDesc* blob_desc = GetBlobDesc4BnInOp(input_diff_bn);
    if (!blob_desc) { continue; }
    blob_desc->set_has_loss_instance_num_field(true);
  }
}

void Operator::FixParallelDesc(ParallelDesc* pr_desc) const { VirtualFixParallelDesc(pr_desc); }

void Operator::FixLbiWhenShareModel(const std::string& shared_op_name) {
  for (const std::string& model_bn : model_bns()) {
    mut_bn_in_op2lbi()->at(model_bn).set_op_name(shared_op_name);
    mut_bn_in_op2lbi()->at(GenDiffBn(model_bn)).set_op_name(shared_op_name);
  }
  for (const std::string& const_model_bn : const_model_bns()) {
    mut_bn_in_op2lbi()->at(const_model_bn).set_op_name(shared_op_name);
  }
}

static bool HasBlobDescWithField(
    std::function<const BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const PbRpf<std::string>& bn_in_ops, bool (BlobDesc::*has_field)() const) {
  for (const std::string& bn_in_op : bn_in_ops) {
    const BlobDesc* blob_desc = GetBlobDesc4BnInOp(bn_in_op);
    if (blob_desc && (blob_desc->*has_field)()) { return true; }
  }
  return false;
}

static bool DoAllBlobDescHaveField(
    std::function<const BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const PbRpf<std::string>& bn_in_ops, bool (BlobDesc::*has_field)() const) {
  for (const std::string& bn_in_op : bn_in_ops) {
    const BlobDesc* blob_desc = GetBlobDesc4BnInOp(bn_in_op);
    if (blob_desc && !(blob_desc->*has_field)()) { return false; }
  }
  return true;
}

static bool HaveSameDim0InnerShape(
    std::function<const BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const PbRpf<std::string>& input_bns, const PbRpf<std::string>& output_bns) {
  auto ForEachBn = [&](const std::function<void(const std::string&)>& Handler) {
    for (const auto& bn : input_bns) { Handler(bn); }
    for (const auto& bn : output_bns) { Handler(bn); }
  };
  bool ret = true;
  std::unique_ptr<Shape> dim0_inner_shape;
  ForEachBn([&](const std::string& bn) {
    if (ret == false) { return; }
    const auto& inner_shape = GetBlobDesc4BnInOp(bn)->dim0_inner_shape();
    if (dim0_inner_shape) {
      if (*dim0_inner_shape != inner_shape) { ret = false; }
    } else {
      dim0_inner_shape.reset(new Shape(inner_shape));
    }
  });
  return ret;
}

ActivationType Operator::GetActivationType() const {
  if (HasFieldInCustomizedConf("activation")) {
    return static_cast<ActivationType>(GetEnumFromCustomizedConf("activation"));
  } else {
    return ActivationType::kNone;
  }
}

void Operator::GenKernelConf(std::function<const BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                             bool is_forward, const ParallelContext* parallel_ctx,
                             KernelConf* kernel_conf, const OpContext* op_ctx) const {
  *(kernel_conf->mutable_op_attribute()) = op_attribute_;
  if (HasBlobDescWithField(GetBlobDesc4BnInOp, output_bns(), &BlobDesc::header_is_opaque)) {
    kernel_conf->set_need_do_opaque_header(true);
  } else {
    if (HasBlobDescWithField(GetBlobDesc4BnInOp, output_bns(), &BlobDesc::has_data_id_field)) {
      kernel_conf->set_need_do_data_id(true);
    }
    const PbRpf<std::string>* bns = &output_bns();
    if (IsLossOp()) { bns = &input_bns(); }
    if (HasBlobDescWithField(GetBlobDesc4BnInOp, *bns, &BlobDesc::has_col_num_field)) {
      kernel_conf->set_need_do_col_num(true);
    }
    if (HasBlobDescWithField(GetBlobDesc4BnInOp, *bns, &BlobDesc::has_dim0_valid_num_field)) {
      kernel_conf->set_need_do_dim0_valid_num(true);
      if (DoAllBlobDescHaveField(GetBlobDesc4BnInOp, input_bns(),
                                 &BlobDesc::has_dim0_valid_num_field)
          && DoAllBlobDescHaveField(GetBlobDesc4BnInOp, output_bns(),
                                    &BlobDesc::has_dim0_valid_num_field)
          && HaveSameDim0InnerShape(GetBlobDesc4BnInOp, input_bns(), output_bns())) {
        kernel_conf->set_can_naive_do_dim0_valid_num(true);
      }
    }
    if (HasBlobDescWithField(GetBlobDesc4BnInOp, *bns, &BlobDesc::has_dim1_valid_num_field)) {
      kernel_conf->set_need_do_dim1_valid_num(true);
    }
    if (HasBlobDescWithField(GetBlobDesc4BnInOp, *bns, &BlobDesc::has_dim2_valid_num_field)) {
      kernel_conf->set_need_do_dim2_valid_num(true);
    }
    if (HasBlobDescWithField(GetBlobDesc4BnInOp, *bns,
                             &BlobDesc::has_record_id_in_device_piece_field)) {
      kernel_conf->set_need_do_record_id_in_device_piece(true);
      if (DoAllBlobDescHaveField(GetBlobDesc4BnInOp, input_bns(),
                                 &BlobDesc::has_record_id_in_device_piece_field)
          && DoAllBlobDescHaveField(GetBlobDesc4BnInOp, output_bns(),
                                    &BlobDesc::has_record_id_in_device_piece_field)) {
        kernel_conf->set_can_naive_do_record_id_in_device_piece(true);
      }
    }
  }

  kernel_conf->set_is_forward(is_forward);
  DataType data_type = GetDataTypeFromBnInOpVec(GetBlobDesc4BnInOp, output_bns());
  if (data_type == DataType::kInvalidDataType) {
    data_type = GetDataTypeFromBnInOpVec(GetBlobDesc4BnInOp, input_bns());
  }
  if (data_type == DataType::kInvalidDataType) {
    data_type = GetDataTypeFromBnInOpVec(GetBlobDesc4BnInOp, output_diff_bns());
  }
  kernel_conf->set_data_type(data_type);

  VirtualGenKernelConf(GetBlobDesc4BnInOp, parallel_ctx, kernel_conf, op_ctx);
}

void Operator::VirtualGenKernelConf(
    std::function<const BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const ParallelContext* parallel_ctx, KernelConf* kernel_conf, const OpContext* op_ctx) const {
  VirtualGenKernelConf(GetBlobDesc4BnInOp, parallel_ctx, kernel_conf);
}

int64_t Operator::cudnn_buf_limit_byte() const {
  int64_t cudnn_buf_limit_mbyte = 0;
  if (op_conf().has_cudnn_buf_limit_mbyte()) {
    cudnn_buf_limit_mbyte = op_conf().cudnn_buf_limit_mbyte();
  } else {
    cudnn_buf_limit_mbyte = Global<JobDesc>::Get()->cudnn_buf_limit_mbyte();
  }
  return cudnn_buf_limit_mbyte * 1024 * 1024;
}

std::string Operator::Bn2ConfName(const std::string& bn) const {
  const PbFd* fd = GetCustomizedConf().GetDescriptor()->FindFieldByName(bn);
  if (fd) {
    return GetValFromCustomizedConf<std::string>(bn);
  } else {
    const std::pair<std::string, int32_t> prefix_idx = GenUnRepeatedBn(bn);
    return GetPbRpfFromCustomizedConf<std::string>(prefix_idx.first).Get(prefix_idx.second);
  }
}

LogicalBlobId Operator::ibn2lbi(const std::string& input_bn) const {
  return GenLogicalBlobId(Bn2ConfName(input_bn));
}
LogicalBlobId Operator::obn2lbi(const std::string& output_bn) const {
  LogicalBlobId ret;
  ret.set_op_name(op_name());
  ret.set_blob_name(Bn2ConfName(output_bn));
  return ret;
}
LogicalBlobId Operator::cmbn2lbi(const std::string& const_model_bn) const {
  LogicalBlobId ret;
  ret.set_op_name(op_name());
  ret.set_blob_name(const_model_bn);
  return ret;
}
LogicalBlobId Operator::cbbn2lbi(const std::string& const_buf_bn) const {
  LogicalBlobId ret;
  ret.set_op_name(op_name());
  ret.set_blob_name(const_buf_bn);
  return ret;
}
LogicalBlobId Operator::mbn2lbi(const std::string& model_bn) const {
  LogicalBlobId ret;
  ret.set_op_name(op_name());
  ret.set_blob_name(model_bn);
  return ret;
}
LogicalBlobId Operator::fwmbn2lbi(const std::string& forward_model_bn) const {
  LogicalBlobId ret;
  ret.set_op_name(op_name());
  ret.set_blob_name(forward_model_bn);
  return ret;
}

void Operator::EnrollDataTmpBn(const std::string& dtbn) {
  *(mut_data_tmp_bns()->Add()) = dtbn;
  CHECK(mut_bn_in_op2lbi()->insert({dtbn, dtbn2lbi(dtbn)}).second);
}

void Operator::EnrollFwBufBn(const std::string& fbbn) {
  *(mut_fw_buf_bns()->Add()) = fbbn;
  CHECK(mut_bn_in_op2lbi()->insert({fbbn, fbbn2lbi(fbbn)}).second);
}

void Operator::EnrollBwBufBn(const std::string& bbbn) {
  *(mut_bw_buf_bns()->Add()) = bbbn;
  CHECK(mut_bn_in_op2lbi()->insert({bbbn, bbbn2lbi(bbbn)}).second);
}

void Operator::EnrollInputBn(const std::string& ibn, bool has_diff) {
  LogicalBlobId lbi = ibn2lbi(ibn);
  *(mut_input_bns()->Add()) = ibn;
  CHECK(mut_bn_in_op2lbi()->insert({ibn, lbi}).second);
  if (has_diff) {
    std::string idbn = GenDiffBn(ibn);
    *(mut_input_diff_bns()->Add()) = idbn;
    CHECK(mut_bn_in_op2lbi()->insert({idbn, lbi}).second);
  }
}

bool Operator::IsIbnMutable(const std::string& ibn) const {
  return mutable_input_bns_.find(ibn) != mutable_input_bns_.end();
}

void Operator::EnrollMutableInputBn(const std::string& ibn) {
  EnrollInputBn(ibn, false);
  mutable_input_bns_.emplace(ibn);
}

void Operator::EnrollRepeatedInputBn(const std::string& ibn_prefix, int32_t num, bool has_diff) {
  FOR_RANGE(int32_t, i, 0, num) { EnrollInputBn(GenRepeatedBn(ibn_prefix, i), has_diff); }
}

void Operator::EnrollRepeatedInputBn(const std::string& ibn_prefix, bool has_diff) {
  EnrollRepeatedInputBn(ibn_prefix, GetPbRpfFromCustomizedConf<std::string>(ibn_prefix).size(),
                        has_diff);
}

void Operator::EnrollRepeatedInputBn(const std::string& ibn_prefix, int32_t num) {
  EnrollRepeatedInputBn(ibn_prefix, num, true);
}

void Operator::EnrollRepeatedInputBn(const std::string& ibn_prefix) {
  EnrollRepeatedInputBn(ibn_prefix, true);
}

void Operator::EnrollOutputBn(const std::string& obn, bool has_diff) {
  LogicalBlobId lbi = obn2lbi(obn);
  *(mut_output_bns()->Add()) = obn;
  CHECK(mut_bn_in_op2lbi()->insert({obn, lbi}).second);
  if (has_diff) {
    std::string odbn = GenDiffBn(obn);
    *(mut_output_diff_bns()->Add()) = odbn;
    CHECK(mut_bn_in_op2lbi()->insert({odbn, lbi}).second);
  }
}

void Operator::EnrollRepeatedOutputBn(const std::string& obn_prefix, int32_t num, bool has_diff) {
  FOR_RANGE(int32_t, i, 0, num) { EnrollOutputBn(GenRepeatedBn(obn_prefix, i), has_diff); }
}

void Operator::EnrollRepeatedOutputBn(const std::string& obn_prefix, bool has_diff) {
  EnrollRepeatedOutputBn(obn_prefix, GetPbRpfFromCustomizedConf<std::string>(obn_prefix).size(),
                         has_diff);
}

void Operator::EnrollRepeatedOutputBn(const std::string& obn_prefix, int32_t num) {
  EnrollRepeatedOutputBn(obn_prefix, num, true);
}

void Operator::EnrollRepeatedOutputBn(const std::string& obn_prefix) {
  EnrollRepeatedOutputBn(obn_prefix, true);
}

void Operator::EnrollModelBn(const std::string& mbn) {
  if (op_conf().trainable() == false) {
    EnrollConstModelBn(mbn);
    return;
  }
  auto Enroll = [&](const std::string& mbn) {
    LogicalBlobId lbi = mbn2lbi(mbn);
    *(mut_model_bns()->Add()) = mbn;
    CHECK(mut_bn_in_op2lbi()->insert({mbn, lbi}).second);
    std::string mdbn = GenDiffBn(mbn);
    *(mut_model_diff_bns()->Add()) = mdbn;
    CHECK(mut_bn_in_op2lbi()->insert({mdbn, lbi}).second);
  };
  Enroll(mbn);
  auto it = op_attribute_.bn_in_op2lbi().find("total_instance_num");
  if (it == op_attribute_.bn_in_op2lbi().end()) { Enroll("total_instance_num"); }
}
void Operator::EnrollModelDiffBn(const std::string& mdbn) {
  LogicalBlobId lbi = mbn2lbi(mdbn);
  *(mut_model_diff_bns()->Add()) = mdbn;
  CHECK(mut_bn_in_op2lbi()->insert({mdbn, lbi}).second);
}
void Operator::EnrollConstModelBn(const std::string& cmbn) {
  *(mut_const_model_bns()->Add()) = cmbn;
  CHECK(mut_bn_in_op2lbi()->insert({cmbn, cmbn2lbi(cmbn)}).second);
}
void Operator::EnrollConstBufBn(const std::string& cbbn) {
  *(mut_const_buf_bns()->Add()) = cbbn;
  CHECK(mut_bn_in_op2lbi()->insert({cbbn, cbbn2lbi(cbbn)}).second);
}
void Operator::EnrollForwardModelBn(const std::string& fwmbn) {
  LogicalBlobId lbi = fwmbn2lbi(fwmbn);
  *(mut_forward_model_bns()->Add()) = fwmbn;
  CHECK(mut_bn_in_op2lbi()->insert({fwmbn, lbi}).second);
}

void Operator::StrFieldTolower(const std::string& field_name) {
  std::string field_val = GetValFromCustomizedConf<std::string>(field_name);
  std::transform(field_val.begin(), field_val.end(), field_val.begin(), ::tolower);
  SetValInCustomizedConf(field_name, field_val);
}

LogicalBlobId Operator::dtbn2lbi(const std::string& data_tmp_bn) const {
  LogicalBlobId lbi;
  lbi.set_op_name(op_name());
  lbi.set_blob_name(data_tmp_bn);
  return lbi;
}

void Operator::InferTotalInstanceNumDesc(
    std::function<BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
    const ParallelContext* parallel_ctx, std::function<void(OpContext*)> EnrollOpCtx) const {
  CHECK_GE(op_attribute_.model_bns().size(), 2);
  auto it = op_attribute_.bn_in_op2lbi().find("total_instance_num");
  if (it != op_attribute_.bn_in_op2lbi().end()) {
    GetBlobDesc4BnInOp("total_instance_num")->mut_shape() = Shape({1});
    for (const std::string& bn : op_attribute_.model_bns()) {
      if (bn != "total_instance_num") {
        GetBlobDesc4BnInOp("total_instance_num")
            ->set_data_type(GetBlobDesc4BnInOp(bn)->data_type());
        break;
      }
    }
  }
}

std::string GenDiffBn(const std::string& bn) { return bn + "_diff"; }
std::string GenUnDiffBn(const std::string& diff_bn) {
  CHECK_STREQ(diff_bn.substr(diff_bn.size() - 5).c_str(), "_diff");
  return diff_bn.substr(0, diff_bn.size() - 5);
}

std::string GenRepeatedBn(const std::string& bn_prefix, int32_t idx) {
  CHECK_GE(idx, 0);
  return bn_prefix + "_" + std::to_string(idx);
}

std::pair<std::string, int32_t> GenUnRepeatedBn(const std::string& bn) {
  const size_t underline_pos = bn.rfind('_');
  CHECK_NE(underline_pos, std::string::npos);
  CHECK_GT(underline_pos, 0);
  CHECK_LT(underline_pos, bn.size() - 1);
  const std::string prefix = bn.substr(0, underline_pos);
  const int32_t idx = oneflow_cast<int32_t>(bn.substr(underline_pos + 1));
  CHECK_GE(idx, 0);
  return std::make_pair(prefix, idx);
}

bool IsOpOnlyCpuSupported(OperatorConf::OpTypeCase op_type_case) {
  return *std::unique_ptr<OnlyCpuSupportPredicator>(NewObj<OnlyCpuSupportPredicator>(op_type_case));
}

std::shared_ptr<Operator> ConstructOp(const OperatorConf& op_conf) {
  Operator* rptr = NewObj<Operator>(op_conf.op_type_case(), op_conf);
  if (IsOpOnlyCpuSupported(op_conf.op_type_case())) {
    CHECK_EQ(op_conf.device_type(), DeviceType::kCPU);
  }
  rptr->InitFromOpConf(op_conf);
  return std::shared_ptr<Operator>(rptr);
}

void EraseEmptyBnInVec(std::function<const BlobDesc*(const std::string&)> GetBlobDesc4BnInOp,
                       PbRpf<std::string>* bns) {
  size_t idx_available = 0;
  for (size_t i = 0; i < bns->size(); ++i) {
    if (GetBlobDesc4BnInOp((*bns)[i])) {
      if (i != idx_available) { (*bns)[idx_available] = (*bns)[i]; }
      ++idx_available;
    }
  }
  bns->erase(bns->begin() + idx_available, bns->end());
}

}  // namespace oneflow
