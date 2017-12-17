#include "easy/runtime/Context.h"

using namespace easy;

void Context::initDefaultArgumentMapping() {
  // identity mapping
  for(size_t i = 0, n = ArgumentMapping_.size(); i != n; ++i) {
    setParameterIndex(i,i);
  }
}

Context& Context::setParameterIndex(unsigned arg_idx, unsigned param_idx) {
  return setArg<ForwardArgument>(arg_idx, param_idx);
}

Context& Context::setParameterInt(unsigned arg_idx, int64_t val) {
  return setArg<IntArgument>(arg_idx, val);
}

Context& Context::setParameterFloat(unsigned arg_idx, double val) {
  return setArg<FloatArgument>(arg_idx, val);
}

Context& Context::setParameterPtrVoid(unsigned arg_idx, const void* val) {
  return setArg<PtrArgument>(arg_idx, val);
}

Context& Context::setParameterPlainStruct(unsigned arg_idx, char const* ptr, size_t size) {
  return setArg<StructArgument>(arg_idx, ptr, size);
}

bool Context::operator==(const Context& Other) const {
  if(getOptLevel() != Other.getOptLevel())
    return false;
  if(size() != Other.size())
    return false;

  for(auto this_it = begin(), other_it = Other.begin();
      this_it != end(); ++this_it, ++other_it) {
    if(!(*this_it == *other_it))
      return false;
  }

  return true;
}
