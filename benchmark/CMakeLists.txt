option(BENCHMARK "Enable benchmarking" OFF)

if(BENCHMARK)
    set(CMAKE_CXX_COMPILER "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/atjitc")

    # NOTE: add sources here
    add_executable(atjit-benchmark benchmark.cpp)

    add_dependencies(atjit-benchmark easy-jit-core)

    find_package(Benchmark REQUIRED)
    include_directories(${Benchmark_INCLUDE_DIRS})
    link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

    target_link_libraries (atjit-benchmark ${Benchmark_LIBRARIES})
    target_link_libraries (atjit-benchmark EasyJitRuntime pthread)
endif()
