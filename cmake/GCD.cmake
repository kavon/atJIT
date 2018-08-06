#################
# find Grand Central Dispatch installation

# defines the $GCD_LIB, and $GCD_INCLUDE_DIR symbols

function (requireFound SYMB MSG)
  if(NOT ${SYMB})
    message(FATAL_ERROR ${MSG})
  endif()
  message(STATUS "Found ${SYMB}: ${${SYMB}}")
endfunction(requireFound)

# search
find_library(GCD_LIB
  NAMES dispatch
)

find_path(GCD_INCLUDE_DIR
  NAMES dispatch/dispatch.h
)

# check
requireFound(GCD_LIB "Grand Central Dispatch shared library (libdispatch) not found")
requireFound(GCD_INCLUDE_DIR "Grand Central Dispatch header files not found")

####################
