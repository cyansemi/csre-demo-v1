configure_file(${CMAKE_CURRENT_SOURCE_DIR}/check_regex.sh.in ${CMAKE_CURRENT_BINARY_DIR}/check_regex.sh @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/check_regex.sh DESTINATION ${INSTALL_DIR})
