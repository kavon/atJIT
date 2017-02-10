cmake_minimum_required(VERSION 2.8.8)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")
message(STATUS "LLVM Root: ${LLVM_TOOLS_BINARY_DIR}")

set(CMAKE_CXX_FLAGS "-std=c++1y -fno-rtti")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

llvm_map_components_to_libnames(llvm_libs all)

add_subdirectory(pass)
add_subdirectory(runtime)
