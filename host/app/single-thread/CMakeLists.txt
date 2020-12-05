add_executable(csre-st ${CORE_SRC} ${REF_SRC} ${RUNTIME_COMMON} ./csre-st.cpp)
target_link_libraries(csre-st recomp ${xrt_core_LIBRARY} uuid boost_system boost_chrono)
target_link_directories(csre-st PRIVATE ${XILINX_XRT}/lib)
target_include_directories(csre-st PRIVATE ${PROJ_ROOT}/host/include)

add_executable(csre-st-ocl ./csre-st-ocl.cpp)
target_link_libraries(csre-st-ocl csre-ocl recomp ${xrt_xilinxopencl_LIBRARY} uuid boost_system boost_chrono)
target_include_directories(csre-st-ocl PRIVATE ${PROJ_ROOT}/host/include)

#install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/csre-st DESTINATION ${INSTALL_DIR})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/csre-st-ocl DESTINATION ${INSTALL_DIR})
