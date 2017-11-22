include(FindPackageHandleStandardArgs)

# allow specifying which Python installation to use
if (NOT PYTHON_EXEC)
  set(PYTHON_EXEC $ENV{PYTHON_EXEC})
endif (NOT PYTHON_EXEC)

if (NOT PYTHON_EXEC)
  find_program(PYTHON_EXEC "python${Python_FIND_VERSION}" 
               DOC "Location of python executable to use")
endif(NOT PYTHON_EXEC)

execute_process(COMMAND "${PYTHON_EXEC}" "-c"
"import sys; print('%d.%d' % (sys.version_info[0],sys.version_info[1]))"
OUTPUT_VARIABLE PYTHON_VERSION
OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE "." "" PYTHON_VERSION_NO_DOTS ${PYTHON_VERSION})


function(find_python_module module)
  string(TOUPPER ${module} module_upper)
  if(NOT PY_${module_upper})
    if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
      set(${module}_FIND_REQUIRED TRUE)
    endif()
  # A module's location is usually a directory, but for binary modules it's a .so file.
    execute_process(COMMAND "${PYTHON_EXEC}" "-c" "import re, ${module}; print re.compile('/__init__.py.*').sub('',${module}.__file__)"
                    RESULT_VARIABLE _${module}_status 
                    OUTPUT_VARIABLE _${module}_location
                    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _${module}_status)
      set(PY_${module_upper} ${_${module}_location} CACHE STRING "Location of Python module ${module}")
    endif(NOT _${module}_status)
  endif(NOT PY_${module_upper})
  find_package_handle_standard_args(PY_${module} DEFAULT_MSG PY_${module_upper})
endfunction(find_python_module)
