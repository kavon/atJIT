llvm_map_components_to_libnames(RUNTIME_LLVM_LIBS all)

add_library(EasyJitRuntime SHARED
  Function.cpp
  Context.cpp
)

target_link_libraries(EasyJitRuntime ${RUNTIME_LLVM_LIBS})

set(EASY_JIT_RUNTIME ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EasyJitRuntime${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

