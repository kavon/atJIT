option(BENCHMARK "Enable benchmarking" OFF)

if(BENCHMARK)
    set(CMAKE_CXX_COMPILER ${LLVM_TOOLS_BINARY_DIR}/clang++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -disable-O0-optnone -Xclang -load -Xclang ${EASY_JIT_PASS}")

    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})

    find_package(Benchmark REQUIRED)
    include_directories(${Benchmark_INCLUDE_DIRS})
    link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

    #########
    add_executable(atjit-benchmark-O0 benchmark.cpp)
    add_dependencies(atjit-benchmark-O0 atjit-core)
    set_target_properties(atjit-benchmark-O0 PROPERTIES COMPILE_FLAGS "-O0")
    target_link_libraries (atjit-benchmark-O0 ${Benchmark_LIBRARIES} ATJitRuntime pthread)
    #########

    #########
    add_executable(atjit-benchmark-O1 benchmark.cpp)
    add_dependencies(atjit-benchmark-O1 atjit-core)
    set_target_properties(atjit-benchmark-O1 PROPERTIES COMPILE_FLAGS "-O1")
    target_link_libraries (atjit-benchmark-O1 ${Benchmark_LIBRARIES} ATJitRuntime pthread)
    #########

    #########
    add_executable(atjit-benchmark-O2 benchmark.cpp)
    add_dependencies(atjit-benchmark-O2 atjit-core)
    set_target_properties(atjit-benchmark-O2 PROPERTIES COMPILE_FLAGS "-O2")
    target_link_libraries (atjit-benchmark-O2 ${Benchmark_LIBRARIES} ATJitRuntime pthread)
    #########

    #########
    add_executable(atjit-benchmark-O3 benchmark.cpp)
    add_dependencies(atjit-benchmark-O3 atjit-core)
    set_target_properties(atjit-benchmark-O3 PROPERTIES COMPILE_FLAGS "-O3")
    target_link_libraries (atjit-benchmark-O3 ${Benchmark_LIBRARIES} ATJitRuntime pthread)
    #########

    if (${POLLY_KNOBS})
      #########
      add_executable(atjit-benchmark-O3p benchmark.cpp)
      add_dependencies(atjit-benchmark-O3p atjit-core)
      set_target_properties(atjit-benchmark-O3p PROPERTIES COMPILE_FLAGS "-O3 -mllvm -polly")
      target_link_libraries (atjit-benchmark-O3p ${Benchmark_LIBRARIES} ATJitRuntime pthread)
      #########
    endif()

endif()
