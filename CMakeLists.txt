cmake_minimum_required(VERSION 3.5)

find_package(LLVM 6.0 REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")
message(STATUS "LLVM Root: ${LLVM_TOOLS_BINARY_DIR}")
message(STATUS "LLVM Include dirs: ${LLVM_INCLUDE_DIRS}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(AddLLVM)
include(Python)
include(XGBoost)

option(LLVM_ENABLE_PLUGINS "Generate build targets for LLVM plugins." ON)
option (FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)


include_directories(SYSTEM
                    ${LLVM_INCLUDE_DIRS}
                    include
                   )

link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})


set(CLANGXX_BIN "${LLVM_TOOLS_BINARY_DIR}/clang++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=return-type")

# needed for colors when using Ninja
# source: https://medium.com/@alasher/colored-c-compiler-output-with-ninja-clang-gcc-10bfe7f2b949
if (${FORCE_COLORED_OUTPUT})
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       add_compile_options (-fdiagnostics-color)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
       add_compile_options (-fcolor-diagnostics)
    endif ()
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(EASY_JIT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(cmake)
add_subdirectory(include)
add_subdirectory(pass)
add_subdirectory(runtime)

add_custom_target(easy-jit-core DEPENDS EasyJitPass EasyJitRuntime)

add_subdirectory(doc)
add_subdirectory(benchmark)

include(CMakeTests.txt)

configure_file("${EASY_JIT_ROOT}/misc/easycc.in" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/easycc")
install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/easycc DESTINATION bin
PERMISSIONS OWNER_EXECUTE OWNER_READ )
