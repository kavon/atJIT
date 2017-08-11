
llvm_map_components_to_libnames(LLVM_LIBS Support Core)

add_llvm_loadable_module(easy_jit_pass
  src/extract.cpp
  src/identify.cpp
  src/declare.cpp)

target_link_libraries(easy_jit_pass PUBLIC ${LLVM_LIBS})

set(EASY_JIT_PASS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/easy_jit_pass${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)
