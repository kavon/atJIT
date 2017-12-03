#include "easy/runtime/Context.h"

using namespace easy;

void Context::initDefaultArgumentMapping() {
  // identity mapping
  for(size_t i = 0, n = ArgumentMapping_.size(); i != n; ++i) {
    ArgumentMapping_[i].ty = Argument::Type::Forward;
    ArgumentMapping_[i].data.param_idx = i;
  }
}

Context& Context::setParameterIndex(unsigned arg_idx, unsigned param_idx) {
  auto &Arg = ArgumentMapping_[arg_idx];
  Arg.ty = Argument::Type::Forward;
  Arg.data.param_idx = param_idx;
  return *this;
}

Context& Context::setParameterInt(unsigned arg_idx, int64_t val) {
  auto &Arg = ArgumentMapping_[arg_idx];
  Arg.ty = Argument::Type::Int;
  Arg.data.integer = val;
  return *this;
}

Context& Context::setParameterFloat(unsigned arg_idx, double val) {
  auto &Arg = ArgumentMapping_[arg_idx];
  Arg.ty = Argument::Type::Float;
  Arg.data.floating = val;
  return *this;
}

Context& Context::setParameterPtrVoid(unsigned arg_idx, void* val) {
  auto &Arg = ArgumentMapping_[arg_idx];
  Arg.ty = Argument::Type::Ptr;
  Arg.data.ptr = val;
  return *this;
}
