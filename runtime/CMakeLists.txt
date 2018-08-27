include(XGBoost)
include(GCD)

add_library(EasyJitRuntime SHARED
  BitcodeTracker.cpp
  Context.cpp
  Function.cpp
  InitNativeTarget.cpp
  Utils.cpp
  pass/ContextAnalysis.cpp
  pass/DevirtualizeConstant.cpp
  pass/InlineParameters.cpp
  tuner/Optimizer.cpp
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
set_target_properties(EasyJitRuntime PROPERTIES
                      INSTALL_RPATH "${XGB_LIB_DIR};${LLVM_LIBRARY_DIR}"
                      INSTALL_RPATH_USE_LINK_PATH TRUE)

include_directories(${GCD_INCLUDE_DIR})



if (${POLLY_KNOBS})

  include(Polly)

  include_directories(${Polly_INCLUDE_DIRS})
  add_definitions(${Polly_DEFINITIONS})

  # Polly's exported targets already includes libLLVM
  target_link_libraries(EasyJitRuntime PUBLIC Polly)

else() #########

find_library(LIBLLVM_LIB
  NAMES LLVM
  PATHS "${LLVM_LIBRARY_DIR}"
  NO_DEFAULT_PATH
)
requireFound(LIBLLVM_LIB "Could not find libLLVM !")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
target_link_libraries(EasyJitRuntime PUBLIC ${LIBLLVM_LIB})

endif()
#########################

target_link_libraries(EasyJitRuntime PUBLIC ${XGB_LIB})
target_link_libraries(EasyJitRuntime PUBLIC ${GCD_LIB})


set(EASY_JIT_RUNTIME ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libEasyJitRuntime${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

install(TARGETS EasyJitRuntime
        LIBRARY DESTINATION lib)
