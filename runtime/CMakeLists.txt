
add_library(EasyJitRuntime SHARED
  BitcodeTracker.cpp
  Context.cpp
  Function.cpp
  InitNativeTarget.cpp
  Utils.cpp
  pass/ContextAnalysis.cpp
  pass/DevirtualizeConstant.cpp
  pass/InlineParameters.cpp
  tuner/TunableInliner.cpp
  tuner/Optimizer.cpp
  tuner/AnalyzingTuner.cpp
  tuner/LoopKnob.cpp
  tuner/LoopSettingGen.cpp
  tuner/KnobConfig.cpp
  tuner/KnobSet.cpp
)

# the installed version of the library needs to
# retain its rpath to the xgboost lib that we're linking in.
# it will get stripped during install without this.
set_target_properties(EasyJitRuntime PROPERTIES
                      INSTALL_RPATH ${XGB_LIB_DIR}
                      INSTALL_RPATH_USE_LINK_PATH TRUE)

target_link_libraries(EasyJitRuntime PUBLIC ${XGB_LIB})

llvm_config(EasyJitRuntime core codegen interpreter support mcjit native executionengine passes objcarcopts)

set(EASY_JIT_RUNTIME ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libEasyJitRuntime${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

install(TARGETS EasyJitRuntime
        LIBRARY DESTINATION lib)
