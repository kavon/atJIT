configure_file(${CMAKE_CURRENT_SOURCE_DIR}/EasyJitConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/EasyJitConfig.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/EasyJitConfig.cmake
        DESTINATION lib/cmake)
