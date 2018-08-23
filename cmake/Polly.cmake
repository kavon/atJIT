#################
# find Polly componenents within LLVM

set(Polly_DIR "${LLVM_ROOT}/lib/cmake/polly")

find_package(Polly REQUIRED CONFIG)

message(STATUS "Using PollyConfig.cmake in ${Polly_DIR}")

# look in PollyConfig.cmake for more info about what's exported

####################
