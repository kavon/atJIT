
llvm_map_components_to_libnames(RUNTIME_LLVM_LIBS all)

add_library(easy_jit_runtime SHARED
  src/runtime.cpp)

target_link_libraries(easy_jit_runtime ${RUNTIME_LLVM_LIBS})

set(EASY_JIT_RUNTIME ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/easy_jit_runtime${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

