#include <tuner/driver.h>

#ifdef POLLY_KNOBS
  #include <polly/RegisterPasses.h>
  #include <polly/ScopDetection.h>
#endif

namespace tuner {

std::atomic<bool> ATDriver::OneTimeInit = false;

void ATDriver::doOneTimeInit() {
#ifdef POLLY_KNOBS

  // quick test
  if (OneTimeInit)
    return;

  bool oldVal = OneTimeInit.exchange(true);

  // did someone else beat me to it?
  if (oldVal)
    return;

  /////////////////////
  // do the one-time initialization

  polly::PollyProcessUnprofitable = true;
  // polly::PollyInvariantLoadHoisting = true; // TODO: should this be on?

  llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
  polly::initializePollyPasses(Registry);

#endif
}


} // end namespace
