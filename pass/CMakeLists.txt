add_llvm_loadable_module(EasyJitPass
  RegisterPasses.cpp
  Easy.cpp
  MayAliasTracer.cpp
  LoopNamer.cpp
  PLUGIN_TOOL
  clang
  )

set(EASY_JIT_PASS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EasyJitPass${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

install(TARGETS EasyJitPass
        LIBRARY DESTINATION lib)
