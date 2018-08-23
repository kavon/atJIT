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

# NOTE:
# We can't use map_components_to_libnames because polly is not currently
# recognized as a component of LLVM. Since we already need libLLVM for a seperate
# issue, we just link that library in instead.
#
# llvm_map_components_to_libnames(llvm_libs core codegen interpreter support mcjit native executionengine passes objcarcopts)

find_library(LIBLLVM_LIB
  NAMES LLVM
  PATHS "${LLVM_LIBRARY_DIR}"
  NO_DEFAULT_PATH
)
requireFound(LIBLLVM_LIB "Could not find libLLVM !")
get_filename_component(LIBLLVM_LIB_DIR ${LIBLLVM_LIB} DIRECTORY)

# the installed version of the library needs to
# retain its rpath to the xgboost and other libs that we're linking in.
# it will get stripped during install without this.
set_target_properties(EasyJitRuntime PROPERTIES
                      INSTALL_RPATH "${XGB_LIB_DIR};${LIBLLVM_LIB_DIR}"
                      INSTALL_RPATH_USE_LINK_PATH TRUE)

include_directories(${GCD_INCLUDE_DIR})

target_link_libraries(EasyJitRuntime ${LIBLLVM_LIB} ${XGB_LIB} ${GCD_LIB})

set(EASY_JIT_RUNTIME ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libEasyJitRuntime${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

install(TARGETS EasyJitRuntime
        LIBRARY DESTINATION lib)
