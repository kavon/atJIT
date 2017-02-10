
llvm_map_components_to_libnames(RUNTIME_LLVM_LIBS all)

add_library(easy_jit_runtime SHARED
  src/runtime.cpp)

target_link_libraries(easy_jit_runtime ${RUNTIME_LLVM_LIBS})

get_property(RUNTIME_TARGET_FILE TARGET easy_jit_runtime PROPERTY LOCATION)
set(EASY_JIT_RUNTIME ${RUNTIME_TARGET_FILE} PARENT_SCOPE)

