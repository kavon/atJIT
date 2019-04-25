#ifndef AT_DRIVER
#define AT_DRIVER

#include <ctime>
#include <cstdlib>
#include <fstream>

#include <easy/jit.h>
#include <unordered_map>

#include <tuner/optimizer.h>
#include <tuner/Util.h>
#include <tuner/JSON.h>

namespace tuner {

  namespace {
    struct OptimizationInfo {
      OptimizationInfo(std::unique_ptr<tuner::Optimizer> Opt_)
          : Opt(std::move(Opt_)),
            Trial(easy::FunctionWrapperBase()),
            Best(easy::FunctionWrapperBase()) {}

      std::unique_ptr<tuner::Optimizer> Opt;
      easy::FunctionWrapperBase Trial;
      easy::FunctionWrapperBase Best;
      std::vector<easy::FunctionWrapperBase> Others;

      // Recurrence where n corresponds to the value of FullExperiments.
      // Thresh(0) = MinDeploy
      // Thresh(n) = Thresh(n-1) + GrowthFactor * Thresh(n-1)
      double DeploymentThresh = EXPERIMENT_MIN_DEPLOY_NS;

      // statistics
      uint64_t Requests = 0; // total requests to reoptimize this function
      uint64_t FullExperiments = 0; // total full (jit) experiments performed
      uint64_t FastExperiments = 0; // total quick swap experiments performed.
      uint64_t BestSwaps = 0; // total number of actual swaps in Fast experiment
    };
  }

class ATDriver {
  using Key = std::pair<void*, std::shared_ptr<easy::Context>>;
  using Entry = OptimizationInfo;

  using iterator = typename std::unordered_map<Key, Entry>::iterator;

  protected:
  std::unordered_map<Key, Entry> DriverState_;
  std::optional<std::string> dumpStats; // std::filesystem not available in GCC 7



  void exportStats(std::string out) {
    std::ofstream file;
    // open in append mode
    file.open(out, std::ios::out | std::ios::ate | std::ios::app);

    // formatting preferences
    file << std::setprecision(10);

    JSON::beginArray(file);
    bool pastFirst = false;
    for (auto const &State : DriverState_) {
      const Key &K = State.first;
      const Entry &E = State.second;

      if (pastFirst)
        JSON::comma(file);

      pastFirst |= true;

      JSON::beginObject(file);

      JSON::output(file, "requests", E.Requests);
      JSON::output(file, "experiments", E.FullExperiments);
      JSON::output(file, "fast_experiments", E.FastExperiments);
      JSON::output(file, "best_swaps", E.BestSwaps);
      JSON::output(file, "deploy_thresh", E.DeploymentThresh);

      E.Opt->dumpStats(file);

      JSON::endObject(file);
    }
    JSON::endArray(file);

    // optimistically assume more arrays will be appended:
    JSON::comma(file);

    file.close();
  }

  public:

  ATDriver() : dumpStats(std::nullopt) { }
  ATDriver(std::string outFile) : dumpStats(outFile) { }

  ~ATDriver() {
    if (dumpStats)
      exportStats(dumpStats.value());
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
    auto &Others = Info.Others;

    bool Impatient = !(OptFromEntry.getContext()->waitForCompile());
    bool WasNotInCache = EmplaceResult.second;

    Info.Requests += 1;

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
    if (!Trial.isEmpty() && Trial.getFeedback().goodQuality()) {
      auto MaybeGood = std::move(Trial);
      Trial = easy::FunctionWrapperBase(); // erase the trial field

      // Check to see which is better
      if (Best.isEmpty() || MaybeGood.getFeedback()
                            .betterThan(
                            Best.getFeedback())) {
        Best = std::move(MaybeGood);
      } else {
        Others.push_back(std::move(MaybeGood));
      }
    }

    // if we are we still evaluating a trial version, return it.
    if (!Trial.isEmpty())
      return reinterpret_cast<wrapper_ty&>(Trial);

    assert(!Best.isEmpty() && "logic error!");

    ///////////
    // otherwise, we're free to make a decision on whether to
    // experiment again, or make use of the best version so far.

    bool deployedLongEnough = Best.getFeedback().getDeployedTime() >= Info.DeploymentThresh;
    bool isNoopTuner = OptFromEntry.isNoopTuner();
    bool shouldReturnBest = isNoopTuner;

    // should we return the current best or not, based on our patience.
    if (!shouldReturnBest && Impatient) {
      if (OptFromEntry.status() == opt_status::Working)
        shouldReturnBest = true; // we're not going to wait
      else
        shouldReturnBest = !deployedLongEnough; // no experiment => return best
    }

    if (!shouldReturnBest) {
      ////////////
      // that means we should experiment by obtaining a totally new version!

      // reset Best's deployment time, since we crossed the limit here.
      Best.getFeedback().resetDeployedTime();

      // raise the deployment threshold
      Info.FullExperiments += 1;
      Info.DeploymentThresh += Info.DeploymentThresh * EXPERIMENT_DEPLOY_GROWTH_RATE;

      Trial = easy::jit_with_optimizer<T, Args...>(OptFromEntry, std::forward<T>(Fun));
      return reinterpret_cast<wrapper_ty&>(Trial);
    }

    ///////////
    // otherwise, quickly return the best version since we don't want to wait.

    if (BEST_SWAP_ENABLE && deployedLongEnough && Others.size() > 0) {
      // We would have experimented, but we were impatient.
      // Next best thing to do is quickly recheck whether the current
      // "best" version is still actually the best, since extensive usage
      // which gives us more accurate measurements.

      Info.FastExperiments += 1;

      double currentBest = Best.getFeedback().avgMeasurement();
      const double INF = std::numeric_limits<double>::max();
      double othersBest = INF;
      size_t othersBestIdx;

      for (size_t i = 0; i < Others.size(); i++) {
        auto &Old = Others[i];
        double oldTime = Old.getFeedback().avgMeasurement();
        if (oldTime < othersBest) {
          othersBest = oldTime;
          othersBestIdx = i;
        }
      }

      if (othersBest != INF) {
        double diff = currentBest - othersBest;
        if (diff >= (currentBest * BEST_SWAP_MARGIN_HUNK)) {
          // Swap out the current best for one of the others.
          Info.BestSwaps += 1;
#ifndef NDEBUG  ///////////////
          std::cout << "best = " << currentBest
                    << ", othersBest[" << othersBestIdx << "] = " << othersBest
                    << ". swapping" << std::endl;
#endif ////////////////////////
          auto OldBest = std::move(Best);
          Best = std::move(Others[othersBestIdx]);
          Best.getFeedback().resetDeployedTime();
          Others[othersBestIdx] = std::move(OldBest);
        }
      }
    } // end of check others for best

    /////
    // finally, return the best version!
    return reinterpret_cast<wrapper_ty&>(Best);

  } // end of reoptimize

}; // end class

} // end namespace

#endif // AT_DRIVER
