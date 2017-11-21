llvm_map_components_to_libnames(LLVM_LIBS Core Support)

add_llvm_loadable_module(EasyJitPass
  Easy.cpp)

target_link_libraries(EasyJitPass ${LLVM_LIBS})

set(EASY_JIT_PASS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EasyJitPass${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)
