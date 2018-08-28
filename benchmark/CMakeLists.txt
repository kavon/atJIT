option(BENCHMARK "Enable benchmarking" OFF)

if(BENCHMARK)
    set(CMAKE_CXX_COMPILER ${LLVM_TOOLS_BINARY_DIR}/clang++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -disable-O0-optnone -Xclang -load -Xclang ${EASY_JIT_PASS} ${LIBLLVM_LINK_FLAGS}")

    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})

    # NOTE: add sources here
    add_executable(atjit-benchmark benchmark.cpp)

    add_dependencies(atjit-benchmark easy-jit-core)

    find_package(Benchmark REQUIRED)
    include_directories(${Benchmark_INCLUDE_DIRS})
    link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

    target_link_libraries (atjit-benchmark ${Benchmark_LIBRARIES})
    target_link_libraries (atjit-benchmark EasyJitRuntime pthread)
endif()
