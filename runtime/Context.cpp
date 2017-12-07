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

bool easy::operator<(easy::Context const &C1, easy::Context const &C2) {
  auto OptC1 = C1.getOptLevel(),
       OptC2 = C2.getOptLevel();
  if(OptC1 < OptC2)
    return true;
  if(OptC1 > OptC2)
    return false;

  size_t SizeC1 = C1.size(), SizeC2 = C2.size();
  if(SizeC1 < SizeC2)
    return true;
  if(SizeC1 > SizeC2)
    return true;

  for(size_t i = 0; i != SizeC1; ++i) {
    auto const &ArgC1 = C1.getArgumentMapping(i);
    auto const &ArgC2 = C2.getArgumentMapping(i);

    if(ArgC1.ty < ArgC2.ty)
      return true;
    if(ArgC1.ty > ArgC2.ty)
      return false;

    // it does not matter anyway if it is integer or not. compare using the biggest size
    if(ArgC1.data.integer < ArgC2.data.integer)
      return true;
    if(ArgC1.data.integer > ArgC2.data.integer)
      return false;
  }
  return false;
}
