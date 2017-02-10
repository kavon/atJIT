
add_library(easy_jit_runtime MODULE
  src/runtime.cpp)

target_link_libraries(easy_jit_runtime ${llvm_libs})
