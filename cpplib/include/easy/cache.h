#ifndef CACHE
#define CACHE

#include <easy/jit.h>
#include <unordered_map>

namespace easy {
class Cache {
  using Key = std::pair<void*, easy::Context>;
  std::unordered_map<Key, easy::FunctionWrapperBase> Cache_;

  public:

  template<class T, class ... Args>
  auto const& EASY_JIT_COMPILER_INTERFACE jit(T &&Fun, Args&& ... args) {
    using wrapper_ty = decltype(easy::jit(std::forward<T>(Fun), std::forward<Args>(args)...));

    void* FunPtr = reinterpret_cast<void*>(meta::get_as_pointer(Fun));
    auto CacheEntry = Cache_.insert(std::make_pair(Key(FunPtr,
                                                       get_context_for<T, Args...>(std::forward<Args>(args)...)),
                                                   FunctionWrapperBase()));
    FunctionWrapperBase &FWB = CacheEntry.first->second;
    if(CacheEntry.second) {
      easy::Context const &C = CacheEntry.first->first.second;
      wrapper_ty FW = easy::jit_with_context<T, Args...>(C, std::forward<T>(Fun));
      FWB = std::move(FW);
    }
    return reinterpret_cast<wrapper_ty&>(FWB);
  }
};
}

#endif // CACHE
