#include "easy/runtime/Context.h"

using namespace easy;

void Context::initDefaultArgumentMapping() {
  // identity mapping
  for(size_t i = 0, n = ArgumentMapping_.size(); i != n; ++i) {
    setParameterIndex(i,i);
  }
}

Context& Context::setParameterIndex(unsigned arg_idx, unsigned param_idx) {
  setArg<ForwardArgument>(arg_idx, param_idx);
  return *this;
}

Context& Context::setParameterInt(unsigned arg_idx, int64_t val) {
  setArg<IntArgument>(arg_idx, val);
  return *this;
}

Context& Context::setParameterFloat(unsigned arg_idx, double val) {
  setArg<FloatArgument>(arg_idx, val);
  return *this;
}

Context& Context::setParameterPtrVoid(unsigned arg_idx, const void* val) {
  setArg<PtrArgument>(arg_idx, val);
  return *this;
}

Context& Context::setParameterPlainStruct(unsigned arg_idx, void const* ptr, size_t size) {
  setArg<StructArgument>(arg_idx, static_cast<const char*>(ptr), size);
  return *this;
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
