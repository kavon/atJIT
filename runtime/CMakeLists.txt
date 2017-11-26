llvm_map_components_to_libnames(RUNTIME_LLVM_LIBS ExecutionEngine TransformUtils ipo Core)

add_library(EasyJitRuntime SHARED
  Function.cpp
  BitcodeTracker.cpp
  Context.cpp
)

target_link_libraries(EasyJitRuntime ${RUNTIME_LLVM_LIBS})

set(EASY_JIT_RUNTIME ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libEasyJitRuntime${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

