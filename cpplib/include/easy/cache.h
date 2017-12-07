#ifndef CACHE
#define CACHE

#include <easy/jit.h>
#include <map>

namespace easy {
class Cache {
  using Key = std::pair<void*, easy::Context>;
  std::map<Key, easy::FunctionWrapperBase> Cache_;

  public:

  template<class T, class ... Args>
  auto const& jit(T &&Fun, Args&& ... args) {
    using wrapper_ty = decltype(easy::jit(std::forward<T>(Fun), std::forward<Args>(args)...));

    void* FunPtr = reinterpret_cast<void*>(meta::get_as_pointer(Fun));
    auto C = get_context_for<T, Args...>(std::forward<Args>(args)...);

    auto CacheEntry = Cache_.insert(std::make_pair(Key(FunPtr, *C), FunctionWrapperBase()));
    FunctionWrapperBase &FWB = CacheEntry.first->second;
    if(CacheEntry.second) {
      wrapper_ty FW = easy::jit_with_context<T, Args...>(std::move(C), std::forward<T>(Fun));
      FWB = std::move(FW);
    }
    return reinterpret_cast<wrapper_ty&>(FWB);
  }
};
}

#endif // CACHE
