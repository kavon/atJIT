#ifndef AT_DRIVER
#define AT_DRIVER

#include <easy/jit.h>
#include <unordered_map>

#include <tuner/optimizer.h>

namespace tuner {

class ATDriver {
  using Key = std::pair<void*, easy::Context>;
  using Entry = std::pair<tuner::Optimizer, easy::FunctionWrapperBase>;

  using iterator = typename std::unordered_map<Key, Entry>::iterator;

  protected:
  std::unordered_map<Key, Entry> Cache_;

  public:

  template<class T, class ... Args>
  auto const& EASY_JIT_COMPILER_INTERFACE reoptimize(T &&Fun, Args&& ... args) {
    void* FunPtr = reinterpret_cast<void*>(easy::meta::get_as_pointer(Fun));
    using wrapper_ty = decltype(easy::jit(std::forward<T>(Fun), std::forward<Args>(args)...));

    easy::Context Cxt = easy::get_context_for<T, Args...>(std::forward<Args>(args)...);

    // FIXME: is it okay to pass Cxt into optimizer by ref and then have a move afterwards?
    auto DummyEntry =
            std::make_pair(tuner::Optimizer(FunPtr, Cxt), easy::FunctionWrapperBase());

    std::pair<iterator, bool> EmplaceResult =
            Cache_.emplace(Key(FunPtr, std::move(Cxt)), std::move(DummyEntry));

    // pull out data from the emplace
    Key const &KeyVals = EmplaceResult.first->first;
    Entry &EntryVals = EmplaceResult.first->second;
    easy::FunctionWrapperBase &FWB = EntryVals.second;
    tuner::Optimizer &Opt = EntryVals.first;
    easy::Context const& CxtFromKey = KeyVals.second;
    // bool WasNotInCache = EmplaceResult.second;

    auto FW = easy::jit_with_context<T, Args...>(CxtFromKey, std::forward<T>(Fun));

    FWB = std::move(FW); // need to keep this alive
    return reinterpret_cast<wrapper_ty&>(FWB);
  }

}; // end class

} // end namespace

#endif // AT_DRIVER
