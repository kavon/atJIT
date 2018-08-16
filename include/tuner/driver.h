#ifndef AT_DRIVER
#define AT_DRIVER

#include <ctime>
#include <cstdlib>

#include <easy/jit.h>
#include <unordered_map>

#include <tuner/optimizer.h>

namespace tuner {

  namespace {
    struct OptimizationInfo {
      OptimizationInfo(std::unique_ptr<tuner::Optimizer> Opt_)
          : Opt(std::move(Opt_)), Trial(easy::FunctionWrapperBase()), Best(easy::FunctionWrapperBase()) {}

      std::unique_ptr<tuner::Optimizer> Opt;
      easy::FunctionWrapperBase Trial;
      easy::FunctionWrapperBase Best;
    };
  }

class ATDriver {
  using Key = std::pair<void*, std::shared_ptr<easy::Context>>;
  using Entry = OptimizationInfo;

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
                                 OptimizationInfo(std::move(Opt)));


    // pull out data from the emplace
    Key const &KeyVals = EmplaceResult.first->first;
    Entry &Info = EmplaceResult.first->second;
    tuner::Optimizer &OptFromEntry = *(Info.Opt);
    easy::FunctionWrapperBase &Trial = Info.Trial;
    easy::FunctionWrapperBase &Best = Info.Best;

    bool WasNotInCache = EmplaceResult.second;

    // decide whether to recompile or not.
    if (WasNotInCache) {
      // this is the first encounter of the function + context
      // we must submit a compilation job
      OptFromEntry.initialize();
      auto FW = easy::jit_with_optimizer<T, Args...>(OptFromEntry, std::forward<T>(Fun));
      Trial = std::move(FW);

    } else if (!Trial.isEmpty() && Trial.getFeedback().avgMeasurement()) {
      // the existing optimized function has recieved enough feedback,
      // so we can proceed to the next trial version.

      // save the trial version if it's the best we've seen.
      if (Best.isEmpty() || Trial.getFeedback()
                            .betterThan(
                            Best.getFeedback())) {
        Best = std::move(Trial);
        Trial = easy::FunctionWrapperBase();
      }

      // auto FW = easy::jit_with_optimizer<T, Args...>(OptFromEntry, std::forward<T>(Fun));
      // Info.Trial = std::move(FW);

    }

    // priority is to give out the trial version if there is one.
    if (Trial.isEmpty()) {
      assert(!Best.isEmpty() && "logic issue");
      return reinterpret_cast<wrapper_ty&>(Best);
    }

    return reinterpret_cast<wrapper_ty&>(Trial);
  }

}; // end class

} // end namespace

#endif // AT_DRIVER
