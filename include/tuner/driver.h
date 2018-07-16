#ifndef AT_DRIVER
#define AT_DRIVER

#include <ctime>
#include <cstdlib>

#include <easy/jit.h>
#include <unordered_map>

#include <tuner/optimizer.h>

namespace tuner {

class ATDriver {
  using Key = std::pair<void*, std::shared_ptr<easy::Context>>;
  using Entry = std::pair<std::unique_ptr<tuner::Optimizer>, easy::FunctionWrapperBase>;

  using iterator = typename std::unordered_map<Key, Entry>::iterator;

  protected:
  std::unordered_map<Key, Entry> DriverState_;

  public:

  template<class T, class ... Args>
  auto const& EASY_JIT_COMPILER_INTERFACE reoptimize(T &&Fun, Args&& ... args) {
    void* FunPtr = reinterpret_cast<void*>(easy::meta::get_as_pointer(Fun));
    using wrapper_ty = decltype(easy::jit(std::forward<T>(Fun), std::forward<Args>(args)...));

    std::shared_ptr<easy::Context> Cxt = easy::get_sharable_context_for<T, Args...>(std::forward<Args>(args)...);

    auto Opt = std::make_unique<tuner::Optimizer>(FunPtr, Cxt, /*LazyInit=*/true);

    std::pair<iterator, bool> EmplaceResult =
            DriverState_.emplace(Key(FunPtr, Cxt),
            std::make_pair(std::move(Opt), easy::FunctionWrapperBase()));


    // pull out data from the emplace
    Key const &KeyVals = EmplaceResult.first->first;
    Entry &EntryVals = EmplaceResult.first->second;
    easy::FunctionWrapperBase &FWB = EntryVals.second;
    tuner::Optimizer &OptFromEntry = *(EntryVals.first);

    bool WasNotInCache = EmplaceResult.second;

    // reasons we will JIT, in order:
    // 1. this is the first encounter of the function + context
    // 2. the existing optimized function has recieved enough feedback,
    //    so the optimizer can make a decision on reoptimizing it.
    if (WasNotInCache || FWB.getFeedback().avgMeasurement()) {
      OptFromEntry.initialize();
      auto FW = easy::jit_with_optimizer<T, Args...>(OptFromEntry, std::forward<T>(Fun));
      FWB = std::move(FW);
    }

    return reinterpret_cast<wrapper_ty&>(FWB);
  }

}; // end class

} // end namespace

#endif // AT_DRIVER
