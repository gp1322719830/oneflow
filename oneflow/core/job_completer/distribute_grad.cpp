#include "oneflow/core/job_completer/autograd.h"

namespace oneflow {

namespace {

void GenerateBackwardOpConf4Concat(
    const Operator& op, std::vector<OperatorConf>* op_confs,
    const std::function<LogicalBlobId*(const std::string&)>& DiffLbi4BnInOp,
    const std::function<const BlobDesc&(const std::string&)>& LogicalBlobDesc4BnInOp) {
  CHECK(op.op_conf().has_distribute_concat_conf());
  const DistributeConcatOpConf& distribute_concat_conf = op.op_conf().distribute_concat_conf();
  OperatorConf split_op;
  split_op.set_name(op.op_conf().name() + "_grad");
  DistributeSplitOpConf* split_op_conf = split_op.mutable_distribute_split_conf();
  split_op_conf->set_in(GenLogicalBlobName(*DiffLbi4BnInOp("out")));
  split_op_conf->set_axis(distribute_concat_conf.axis());
  FOR_RANGE(int32_t, i, 0, distribute_concat_conf.in_size()) {
    const std::string& ibn_of_distribute_concat_op = op.input_bns().Get(i);
    const std::string& obn = "out_" + std::to_string(i);
    split_op_conf->add_out(obn);
    if (DiffLbi4BnInOp(ibn_of_distribute_concat_op) != nullptr) {
      DiffLbi4BnInOp(ibn_of_distribute_concat_op)->set_op_name(split_op.name());
      DiffLbi4BnInOp(ibn_of_distribute_concat_op)->set_blob_name(obn);
    }
  }
  op_confs->push_back(split_op);
}

void GenerateBackwardOpConf4Split(
    const Operator& op, std::vector<OperatorConf>* op_confs,
    const std::function<LogicalBlobId*(const std::string&)>& DiffLbi4BnInOp,
    const std::function<const BlobDesc&(const std::string&)>& LogicalBlobDesc4BnInOp) {
  CHECK(op.op_conf().has_distribute_split_conf());
  const DistributeSplitOpConf& distribute_split_conf = op.op_conf().distribute_split_conf();
  OperatorConf concat_op;
  concat_op.set_name(op.op_conf().name() + "_grad");
  DistributeConcatOpConf* concat_op_conf = concat_op.mutable_distribute_concat_conf();
  concat_op_conf->set_axis(distribute_split_conf.axis());
  const bool has_diff_0 = DiffLbi4BnInOp(op.output_bns().Get(0)) != nullptr;
  FOR_RANGE(int32_t, i, 0, distribute_split_conf.out_size()) {
    const std::string& obn_of_distribute_split_op = op.output_bns().Get(i);
    const bool has_diff_i = DiffLbi4BnInOp(obn_of_distribute_split_op) != nullptr;
    CHECK_EQ(has_diff_i, has_diff_0);
    if (has_diff_i) {
      concat_op_conf->add_in(GenLogicalBlobName(*DiffLbi4BnInOp(obn_of_distribute_split_op)));
    }
  }
  concat_op_conf->set_out("out");
  if (DiffLbi4BnInOp("in") != nullptr) {
    CHECK_EQ(concat_op_conf->in_size(), distribute_split_conf.out_size());
    DiffLbi4BnInOp("in")->set_op_name(concat_op.name());
    DiffLbi4BnInOp("in")->set_blob_name("out");
    op_confs->push_back(concat_op);
  }
}

void GenerateBackwardOpConf4Broadcast(
    const Operator& op, std::vector<OperatorConf>* op_confs,
    const std::function<LogicalBlobId*(const std::string&)>& DiffLbi4BnInOp,
    const std::function<const BlobDesc&(const std::string&)>& LogicalBlobDesc4BnInOp) {
  CHECK(op.op_conf().has_distribute_broadcast_conf());
  const DistributeBroadcastOpConf& conf = op.op_conf().distribute_broadcast_conf();
  OperatorConf partial_op;
  partial_op.set_name(op.op_conf().name() + "_grad");
  DistributePartialOpConf* partial_op_conf = partial_op.mutable_distribute_partial_conf();
  const bool has_diff_0 = DiffLbi4BnInOp(op.output_bns().Get(0)) != nullptr;
  FOR_RANGE(int32_t, i, 0, conf.out_size()) {
    const std::string& obn_of_distribute_broadcast_op = op.output_bns().Get(i);
    const bool has_diff_i = DiffLbi4BnInOp(obn_of_distribute_broadcast_op) != nullptr;
    CHECK_EQ(has_diff_i, has_diff_0);
    if (has_diff_i) {
      partial_op_conf->add_in(GenLogicalBlobName(*DiffLbi4BnInOp(obn_of_distribute_broadcast_op)));
    }
  }
  partial_op_conf->set_out("out");
  if (DiffLbi4BnInOp("in") != nullptr) {
    CHECK_EQ(partial_op_conf->in_size(), conf.out_size());
    DiffLbi4BnInOp("in")->set_op_name(partial_op.name());
    DiffLbi4BnInOp("in")->set_blob_name("out");
    op_confs->push_back(partial_op);
  }
}

void GenerateBackwardOpConf4Partial(
    const Operator& op, std::vector<OperatorConf>* op_confs,
    const std::function<LogicalBlobId*(const std::string&)>& DiffLbi4BnInOp,
    const std::function<const BlobDesc&(const std::string&)>& LogicalBlobDesc4BnInOp) {
  CHECK(op.op_conf().has_distribute_partial_conf());
  const auto& distribute_partial_conf = op.op_conf().distribute_partial_conf();
  OperatorConf broadcast_op;
  broadcast_op.set_name(op.op_conf().name() + "_grad");
  DistributeBroadcastOpConf* broadcast_op_conf = broadcast_op.mutable_distribute_broadcast_conf();
  broadcast_op_conf->set_in(GenLogicalBlobName(*DiffLbi4BnInOp("out")));
  FOR_RANGE(int32_t, i, 0, distribute_partial_conf.in_size()) {
    const std::string& ibn_of_distribute_partial_op = op.input_bns().Get(i);
    const std::string& obn = "out_" + std::to_string(i);
    broadcast_op_conf->add_out(obn);
    if (DiffLbi4BnInOp(ibn_of_distribute_partial_op) != nullptr) {
      DiffLbi4BnInOp(ibn_of_distribute_partial_op)->set_op_name(broadcast_op.name());
      DiffLbi4BnInOp(ibn_of_distribute_partial_op)->set_blob_name(obn);
    }
  }
  op_confs->push_back(broadcast_op);
}

}  // namespace

REGISTER_OP_GRAD(OperatorConf::kDistributeConcatConf, &GenerateBackwardOpConf4Concat);
REGISTER_OP_GRAD(OperatorConf::kDistributeSplitConf, &GenerateBackwardOpConf4Split);
REGISTER_OP_GRAD(OperatorConf::kDistributeBroadcastConf, &GenerateBackwardOpConf4Broadcast);
REGISTER_OP_GRAD(OperatorConf::kDistributePartialConf, &GenerateBackwardOpConf4Partial);

}  // namespace oneflow
