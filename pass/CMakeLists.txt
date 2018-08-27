include(AddLLVM)

add_llvm_loadable_module(EasyJitPass
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
