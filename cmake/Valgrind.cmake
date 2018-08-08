#################
# find valgrind executable

# defines the $VALGRIND_EXE symbol

function (requireFound SYMB MSG)
  if(NOT ${SYMB})
    message(FATAL_ERROR ${MSG})
  endif()
  message(STATUS "Found ${SYMB}: ${${SYMB}}")
endfunction(requireFound)

# search
find_program(VALGRIND_EXE valgrind)

# check
requireFound(VALGRIND_EXE "Valgrind executable not found")

####################
