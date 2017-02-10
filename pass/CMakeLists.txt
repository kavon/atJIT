
add_library(easy_jit_cc MODULE
	src/extract.cpp)

target_link_libraries(easy_jit_cc ${llvm_libs})
