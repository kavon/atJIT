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
  int ticket = -1;

  public:

  ATDriver() {
    // std::cerr << "Create ATDriver\n";
  }

  ~ATDriver() {
    // std::cerr << "Destroy ATDriver\n";
  }

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

    bool Impatient = !(OptFromEntry.getContext()->waitForCompile());
    bool WasNotInCache = EmplaceResult.second;

    ////////
    // decide whether to recompile or not.

    if (WasNotInCache) {
      // this is the first encounter of the function + context
      // we must submit a compilation job
      OptFromEntry.initialize();
      Trial = easy::jit_with_optimizer<T, Args...>(OptFromEntry, std::forward<T>(Fun));
      return reinterpret_cast<wrapper_ty&>(Trial);
    }

    // Check if the trial version is done.
    if (!Trial.isEmpty() && Trial.getFeedback().avgMeasurement()) {
      auto MaybeGood = std::move(Trial);
      Trial = easy::FunctionWrapperBase();

      // Check to see which is better
      if (Best.isEmpty() || MaybeGood.getFeedback()
                            .betterThan(
                            Best.getFeedback())) {
        Best = std::move(MaybeGood);
      }
    }

    // are we still evaluating a trial version?
    if (Trial.isEmpty()) {
      bool shouldReturnBest = false;

      if (Impatient) {
        if (OptFromEntry.status() == opt_status::Working) {
          shouldReturnBest = true; // we're not going to wait
        } else {
          ticket = (ticket + 1) % EXPERIMENT_RATE; // take a ticket
          shouldReturnBest = (ticket != 0); // 0 indicates doing an experiment
        }
      }

      if (shouldReturnBest) {
        assert(!Best.isEmpty() && "logic error!");
        return reinterpret_cast<wrapper_ty&>(Best);
      }

      // Either the optimizer has nothing available and is inactive,
      // or we have something ready to go. Either way, get a new version.
      Trial = easy::jit_with_optimizer<T, Args...>(OptFromEntry, std::forward<T>(Fun));
      return reinterpret_cast<wrapper_ty&>(Trial);
    }

    assert(!Trial.isEmpty() && "error");

    // trial version is not done, return it
    return reinterpret_cast<wrapper_ty&>(Trial);
  }

}; // end class

} // end namespace

#endif // AT_DRIVER
