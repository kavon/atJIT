
llvm_map_components_to_libnames(LLVM_LIBS all)

add_llvm_loadable_module(easy_jit_pass src/extract.cpp src/identify.cpp src/declare.cpp)

target_link_libraries(easy_jit_pass PUBLIC ${LLVM_LIBS})

get_property(PASS_TARGET_FILE TARGET easy_jit_pass PROPERTY LOCATION)
set(EASY_JIT_PASS ${PASS_TARGET_FILE} PARENT_SCOPE)
