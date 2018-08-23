cmake_minimum_required(VERSION 3.5)

option(LLVM_ENABLE_PLUGINS "Generate build targets for LLVM plugins." ON)
option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)
option(POLLY_KNOBS "Enable the use of Polly knobs" ON)


if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

############################################
#### identify and configure LLVM

if (DEFINED LLVM_ROOT)
  set(LLVM_DIR "${LLVM_ROOT}/lib/cmake/llvm")
endif()

if (NOT (IS_ABSOLUTE ${LLVM_DIR}))
    message(FATAL_ERROR "LLVM_DIR / LLVM_ROOT must be an absolute path. Tried dir: ${LLVM_DIR}")
endif()

find_package(LLVM REQUIRED CONFIG
              NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in ${LLVM_DIR}")

if (${LLVM_VERSION_MAJOR} LESS 6)
  message(FATAL_ERROR "atJIT is not known to support LLVM < 6")
endif()

if (${LLVM_ENABLE_RTTI} EQUAL "ON")
  message(FATAL_ERROR "atJIT currently requires LLVM with RTTI.")
endif()

## configure compilation flag and atjitc script for polly
if (${POLLY_KNOBS})
  message(STATUS "Polly Knobs: ON")
  add_definitions(-DPOLLY_KNOBS)
  set(LIBLLVM_LINK_FLAGS "-Wl,-rpath,'${LLVM_LIBRARY_DIR}' -L${LLVM_LIBRARY_DIR} -lLLVM")
else()
  message(STATUS "Polly Knobs: OFF")
  set(LIBLLVM_LINK_FLAGS "")
endif()


list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(Python)
include(XGBoost)
include(GCD)

include_directories(SYSTEM
                    ${LLVM_INCLUDE_DIRS}
                    include
                   )

add_definitions(${LLVM_DEFINITIONS})
add_definitions(-DLLVM_VERSION_MAJOR=${LLVM_VERSION_MAJOR})


set(CLANGXX_BIN "${LLVM_TOOLS_BINARY_DIR}/clang++")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror=return-type -Wno-unused-parameter")

# NOTE: -O2 and -O3 cause the plugin to segfault _only_ on Travis CI with LLVM 6.
# it happens when the bitcode register pass tries to construct a Twine, so
# there's no straight-forward fix.
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O1")

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

configure_file("${EASY_JIT_ROOT}/misc/atjitc.in" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/atjitc")
install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/atjitc DESTINATION bin
PERMISSIONS OWNER_EXECUTE OWNER_READ )
