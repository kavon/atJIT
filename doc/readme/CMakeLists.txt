option(ATJIT_EXAMPLE "Build Examples" OFF)

if(ATJIT_EXAMPLE)
    find_package(OpenCV REQUIRED)

    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS})

    set(CMAKE_CXX_COMPILER ${LLVM_TOOLS_BINARY_DIR}/clang++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -disable-O0-optnone -Xclang -load -Xclang ${EASY_JIT_PASS}")

    add_executable(easyjit-example camfilter.cpp)
    add_dependencies(easyjit-example atjit-core)

    include_directories(${OpenCV_INCLUDE_DIRS})
    target_link_libraries(easyjit-example ${OpenCV_LIBS})

    link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

    target_link_libraries (easyjit-example ${Benchmark_LIBRARIES})
    target_link_libraries (easyjit-example ATJitRuntime pthread)
endif()
