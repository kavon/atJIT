#################
# find jsonlint executable

# defines the $JSONLINT_EXE symbol

function (requireFound SYMB MSG)
  if(NOT ${SYMB})
    message(FATAL_ERROR ${MSG})
  endif()
  message(STATUS "Found ${SYMB}: ${${SYMB}}")
endfunction(requireFound)

# search
find_program(JSONLINT_EXE NAMES jsonlint jsonlint-py3 jsonlint-py)

# check
requireFound(JSONLINT_EXE "jsonlint executable not found")

####################
