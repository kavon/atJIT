#################
# find xgboost installation

# this script automatically includes all needed include dirs,
# and outputs the following variables:
#
#   XGB_LIB -- an absolute path to the shared library file for xgboost
#

function (requireFound SYMB MSG)
  if(NOT ${SYMB})
    message(FATAL_ERROR ${MSG})
  endif()
  message(STATUS "Found ${SYMB}: ${${SYMB}}")
endfunction(requireFound)

# search
find_library(XGB_LIB
  NAMES xgboost
  HINTS "${PROJECT_SOURCE_DIR}/xgboost/root/lib"
)

find_path(XGB_INCLUDE_DIR
  NAMES xgboost/base.h
  HINTS "${PROJECT_SOURCE_DIR}/xgboost/root/include"
)

find_path(RABIT_INCLUDE_DIR
  NAMES rabit/rabit.h
  HINTS "${PROJECT_SOURCE_DIR}/xgboost/root/rabit/include"
)

find_path(DMLC_INCLUDE_DIR
  NAMES dmlc/omp.h
  HINTS "${PROJECT_SOURCE_DIR}/xgboost/root/dmlc-core/include"
)

# check
requireFound(XGB_LIB "XGBoost shared library (libxgboost) not found")
requireFound(XGB_INCLUDE_DIR "XGBoost include header files not found")
requireFound(RABIT_INCLUDE_DIR "Rabit include header files not found")
requireFound(DMLC_INCLUDE_DIR "DMLC include header files not found")

include_directories(${XGB_INCLUDE_DIR}
                    ${RABIT_INCLUDE_DIR}
                    ${DMLC_INCLUDE_DIR}
                  )

####################
