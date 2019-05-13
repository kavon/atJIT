cmake_minimum_required(VERSION 3.4.3)

project(test)

find_package(EasyJit REQUIRED CONFIG)

message("atJIT include dir: " ${ATJit_INCLUDE_DIRS})
message("atJIT lib dir: " ${ATJit_LIBRARY_DIRS})
message("atJIT runtime: " ${ATJit_LIBRARY})
message("atJIT plugin: " ${ATJit_PLUGIN})

find_package(LLVM 6.0 REQUIRED CONFIG)

message(STATUS "LLVM Include dirs: ${LLVM_INCLUDE_DIRS}")

include_directories(${ATJit_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
link_directories(${ATJit_LIBRARY_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++17 -Xclang -disable-O0-optnone -Xclang -load -Xclang ${ATJit_PLUGIN} -Wl,-rpath,'${LLVM_LIBRARY_DIR}' -L${LLVM_LIBRARY_DIR} -lLLVM")

add_executable(InstallTest
  test.cpp
)

target_link_libraries(InstallTest ${ATJit_LIBRARY})
