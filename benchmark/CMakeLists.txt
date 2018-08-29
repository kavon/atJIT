option(BENCHMARK "Enable benchmarking" OFF)

if(BENCHMARK)
    set(CMAKE_CXX_COMPILER ${LLVM_TOOLS_BINARY_DIR}/clang++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -disable-O0-optnone -Xclang -load -Xclang ${EASY_JIT_PASS}")

    # FIXME: produce multiple versions of the
    # benchmark binary, each compiled with diff opt levels.
    # my theory is that it's best to compare O3-compiled
    # non-tuning versian agianst O1 or O0 tuned version.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1")

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
