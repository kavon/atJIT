cmake_minimum_required(VERSION 3.4.3)

find_package(LLVM 5.0 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")
message(STATUS "LLVM Root: ${LLVM_TOOLS_BINARY_DIR}")
message(STATUS "LLVM Include dirs: ${LLVM_INCLUDE_DIRS}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(AddLLVM)
include(Python)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(cpplib/include)

add_compile_options(-std=c++14)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(EASY_JIT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(cpplib)
add_subdirectory(pass)
add_subdirectory(runtime)

add_custom_target(easy-jit-core DEPENDS EasyJitPass EasyJitRuntime)

add_subdirectory(doc)
add_subdirectory(benchmark)

include(CMakeTests.txt)
