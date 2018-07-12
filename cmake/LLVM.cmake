
# The runtime system uses RTTI, and I'm too lazy
# to convert to LLVM's RTTI system.
set(LLVM_ENABLE_RTTI "ON" CACHE BOOL "")

# we use things like Filecheck in our test suite.
set(LLVM_INSTALL_UTILS "ON" CACHE BOOL "")

# see https://github.com/kavon/atJIT/issues/1
set(LLVM_LINK_LLVM_DYLIB "ON" CACHE BOOL "")
