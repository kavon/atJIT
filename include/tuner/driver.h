#ifndef AT_DRIVER
#define AT_DRIVER

#include <easy/jit.h>
#include <unordered_map>

#include <tuner/Tuner.h>

namespace tuner {

class ATDriver {
  using Key = std::pair<void*, easy::Context>;
  using Entry = std::pair<tuner::TunerBase, easy::FunctionWrapperBase>;

  using iterator = typename std::unordered_map<Key, Entry>::iterator;

  protected:
  std::unordered_map<Key, Entry> Cache_;

  template<class T, class ... Args>
  auto const & maybe_recompile(std::pair<iterator, bool> &EmplaceResult, T &&Fun, Args&& ... args) {
    using wrapper_ty = decltype(easy::jit(std::forward<T>(Fun), std::forward<Args>(args)...));

    bool WasNotInCache = EmplaceResult.second;

    Key const &KeyVals = EmplaceResult.first->first;
    Entry &EntryVals = EmplaceResult.first->second;

    easy::FunctionWrapperBase &FWB = EntryVals.second;
    tuner::TunerBase &ATB = EntryVals.first;

    easy::Context const& Cxt = KeyVals.second;

    if(WasNotInCache) {
      // TODO construct a fresh tuner requested in the Context
      // ATB = std::move(AT)
    }

    // TODO dispatch to a jit_with_autotuning, which will ask the tuner
    // whether recompilation is needed.
    auto FW = easy::jit_with_context(Cxt, std::forward<T>(Fun));

    FWB = std::move(FW); // need to keep this alive
    return reinterpret_cast<wrapper_ty&>(FWB);
  }



  public:

  template<class T, class ... Args>
  auto const& EASY_JIT_COMPILER_INTERFACE reoptimize(T &&Fun, Args&& ... args) {
    void* FunPtr = reinterpret_cast<void*>(easy::meta::get_as_pointer(Fun));

    easy::Context Cxt = easy::get_context_for<T, Args...>(std::forward<Args>(args)...);
    auto DummyEntry = std::make_pair(tuner::TunerBase(), easy::FunctionWrapperBase());

    auto CacheEntry = Cache_.emplace(Key(FunPtr, std::move(Cxt)), std::move(DummyEntry));

    return maybe_recompile(CacheEntry, std::forward<T>(Fun), std::forward<Args>(args)...);
  }

}; // end class

} // end namespace

#endif // AT_DRIVER
