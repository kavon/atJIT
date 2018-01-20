option(EASY_JIT_BENCHMARK "Enable benchmarking" OFF)

if(EASY_JIT_BENCHMARK)
    set(CMAKE_CXX_COMPILER ${LLVM_TOOLS_BINARY_DIR}/clang++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -disable-O0-optnone -Xclang -load -Xclang ${EASY_JIT_PASS}")

    add_executable(easyjit-benchmark benchmark.cpp)
    add_dependencies(easyjit-benchmark easy-jit-core)

    find_package(Benchmark REQUIRED)
    include_directories(${Benchmark_INCLUDE_DIRS})
    link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

    target_link_libraries (easyjit-benchmark ${Benchmark_LIBRARIES})
    target_link_libraries (easyjit-benchmark EasyJitRuntime pthread)
endif()
