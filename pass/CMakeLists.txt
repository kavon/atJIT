# NOTE: -O2 and -O3 cause the plugin to segfault _only_ on Travis CI with LLVM 6.
# it happens when the bitcode register pass tries to construct a Twine, so
# there's no straight-forward fix.
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1")

include(AddLLVM)

add_llvm_library(EasyJitPass
  MODULE
  RegisterPasses.cpp
  Easy.cpp
  MayAliasTracer.cpp
  LoopNamer.cpp
  PLUGIN_TOOL
  clang
  )

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(EASY_JIT_PASS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EasyJitPass${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

install(TARGETS EasyJitPass
        LIBRARY DESTINATION lib)
