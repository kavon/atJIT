set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# enable logging with streams with loguru
add_definitions(-DLOGURU_WITH_STREAMS=1)

include(XGBoost)
include(GCD)

add_library(ATJitRuntime SHARED
  BitcodeTracker.cpp
  Context.cpp
  Function.cpp
  InitNativeTarget.cpp
  Utils.cpp
  loguru.cpp
  pass/ContextAnalysis.cpp
  pass/DevirtualizeConstant.cpp
  pass/InlineParameters.cpp
  tuner/Optimizer.cpp
  tuner/Feedback.cpp
  tuner/AnalyzingTuner.cpp
  tuner/LoopKnob.cpp
  tuner/LoopSettingGen.cpp
  tuner/KnobConfig.cpp
  tuner/KnobSet.cpp
  tuner/Statics.cpp
  tuner/Knob.cpp
  tuner/Util.cpp
)

# the installed version of the library needs to
# retain its rpath to the non-system-wide libs that we're linking in.
# it will get stripped during install without this.
set_target_properties(ATJitRuntime PROPERTIES
                      INSTALL_RPATH "${XGB_LIB_DIR};${LLVM_LIBRARY_DIR}"
                      INSTALL_RPATH_USE_LINK_PATH TRUE)

include_directories(${GCD_INCLUDE_DIR})



if (${POLLY_KNOBS})

  include(Polly)

  include_directories(${Polly_INCLUDE_DIRS})
  add_definitions(${Polly_DEFINITIONS})

  # Polly's exported targets already includes libLLVM
  target_link_libraries(ATJitRuntime PUBLIC Polly)

else() #########

find_library(LIBLLVM_LIB
  NAMES LLVM
  PATHS "${LLVM_LIBRARY_DIR}"
  NO_DEFAULT_PATH
)
requireFound(LIBLLVM_LIB "Could not find libLLVM !")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
target_link_libraries(ATJitRuntime PUBLIC ${LIBLLVM_LIB})

endif()
#########################

target_link_libraries(ATJitRuntime PUBLIC ${XGB_LIB})
target_link_libraries(ATJitRuntime PUBLIC ${GCD_LIB})


set(ATJIT_RUNTIME ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libATJitRuntime${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

install(TARGETS ATJitRuntime
        LIBRARY DESTINATION lib)

configure_file("${ATJIT_ROOT}/misc/atjitc.in" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/atjitc")
install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/atjitc DESTINATION bin
PERMISSIONS OWNER_EXECUTE OWNER_READ )
