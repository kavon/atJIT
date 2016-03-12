cmake_minimum_required(VERSION 2.8.8)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")
message(STATUS "LLVM Root: ${LLVM_TOOLS_BINARY_DIR}")

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "-std=c++1y -fno-rtti")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ./bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ./bin)

add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})
include_directories(${LLVM_INCLUDE_DIRS} include)

llvm_map_components_to_libnames(llvm_libs )

message(STATUS "CXX flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Linking with: ${llvm_libs}")

add_library(extract_pass MODULE
	src/extract.cpp)

target_link_libraries(extract_pass ${llvm_libs})
