include(cmake/env.cmake)
include(cmake/macros.cmake)

project(csre)

set(CMAKE_VXX_COMPILER ${XILINX_VITIS}/bin/v++)
set(EMCONFIGUTIL ${XILINX_VITIS}/bin/emconfigutil)
set(XCLBINUTIL ${XILINX_XRT}/bin/xclbinutil)
set(VIVADO ${XILINX_VIVADO}/bin/vivado)
set(GIT git)
set(CSRE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/string-match)
set(SCRIPT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
set(SCRIPT_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/scripts)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SRC_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)
set(BUILD_TMP ${CMAKE_CURRENT_BINARY_DIR}/build_dir.${XCL_TARGET}.${XCL_XSA})

file(MAKE_DIRECTORY ${SRC_BIN_DIR})
file(MAKE_DIRECTORY ${BUILD_TMP})
file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/string-match ${SRC_BIN_DIR}/string-match SYMBOLIC)

# Get the git head hash to identify which commit of string-match we are working on
execute_process(
    COMMAND ${GIT} rev-parse --short HEAD
    WORKING_DIRECTORY ${SRC_BIN_DIR}/string-match
    OUTPUT_VARIABLE CSRE_GIT_HEAD_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

configure_file(${CSRE_SRC}/ip/create_all_ips.tcl ${SCRIPT_BIN_DIR}/create_all_ips.tcl COPYONLY)
configure_file(${SCRIPT_DIR}/gen_csre_xo.tcl ${SCRIPT_BIN_DIR}/gen_csre_xo.tcl COPYONLY)
configure_file(${SCRIPT_DIR}/add_ila.tcl ${SCRIPT_BIN_DIR}/add_ila.tcl COPYONLY)

configure_file(${SCRIPT_DIR}/package_kernel.tcl ${SCRIPT_BIN_DIR}/package_kernel.tcl COPYONLY)
configure_file(${SRC_DIR}/csre_kernel_wrapper.v.in
    ${SRC_BIN_DIR}/csre_kernel_wrapper_${CSRE_MAX_CHAR_NUM}_${CSRE_MAX_STATE_NUM}_${CSRE_GIT_HEAD_HASH}.v)
configure_file(${SRC_DIR}/gen_vitis_config.sh.in ${SRC_BIN_DIR}/gen_vitis_config.sh @ONLY)

file(GLOB_RECURSE CSRE_DESIGN_CORE_FILES "${CSRE_SRC}/hdl/core/*.v")
file(GLOB_RECURSE CSRE_DESIGN_SHIM_FILES "${CSRE_SRC}/hdl/shim/*.v")
set(CSRE_DESIGN_FILES ${CSRE_DESIGN_SHIM_FILES} ${CSRE_DESIGN_CORE_FILES})

# Generate XO for each kernel
foreach(KERNEL_ID RANGE 1 ${KERNEL_NUM})

    set(KERNEL_NAME ${PROJECT_NAME}_${KERNEL_ID})
    set(OBJ_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${KERNEL_NAME}.xo)
    configure_file(${SRC_DIR}/kernel.xml.in ${SRC_BIN_DIR}/kernel_${KERNEL_ID}.xml)

    add_custom_command(
        OUTPUT ${OBJ_FILENAME}

        COMMAND ${VIVADO}
        -mode batch
        -source ${SCRIPT_BIN_DIR}/gen_csre_xo.tcl 
        -tclargs
        ${OBJ_FILENAME}
        ${KERNEL_NAME}
        ${XCL_TARGET}
        ${XCL_PLATFORM}
        ${XCL_XSA}
        ${KERNEL_ID}

        COMMENT "Compiling target: ${OBJ_FILENAME}"

        DEPENDS ${SCRIPT_BIN_DIR}/create_all_ips.tcl
        DEPENDS ${SCRIPT_BIN_DIR}/package_kernel.tcl
        DEPENDS ${SRC_BIN_DIR}/${XCL_XSA}.ini
        DEPENDS ${SRC_BIN_DIR}/kernel_${KERNEL_ID}.xml
        DEPENDS ${CSRE_DESIGN_FILES}
        )

    set(OBJ_FILENAMES ${OBJ_FILENAMES} ${OBJ_FILENAME})
endforeach()

message("--------> ${KERNEL_NUM} kernels configured <--------")

# Generate v++ configuration files
add_custom_command(
    OUTPUT ${SRC_BIN_DIR}/${XCL_XSA}.ini
    OUTPUT ${SRC_BIN_DIR}/vivado_opt.ini
    COMMAND ${SRC_BIN_DIR}/gen_vitis_config.sh ${KERNEL_NUM} ${XCL_XSA}
    DEPENDS ${SRC_BIN_DIR}/gen_vitis_config.sh
    WORKING_DIRECTORY ${SRC_BIN_DIR}
    )

add_custom_target(${PROJECT_NAME}
    DEPENDS ${OBJ_FILENAMES}
    )

set(CFG_FILENAME ${SRC_BIN_DIR}/${XCL_XSA}.ini)
set(VIVADO_OPT_FILENAME ${SRC_BIN_DIR}/vivado_opt.ini)
set(BIN_FILENAME ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.xclbin)
set(FINAL_BIN_FILENAME
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}_kn${KERNEL_NUM}_freq${KERNEL_FREQ}_char${CSRE_MAX_CHAR_NUM}_state${CSRE_MAX_STATE_NUM}_count${CSRE_MAX_COUNT_NUM}_${CSRE_GIT_HEAD_HASH}.xclbin)
if (${XCL_TARGET} STREQUAL "hw_emu")
    set(VXX_FLAGS "-g")
endif()

# Link XO together. This is where the real FPGA implementation happens
add_custom_command(
    OUTPUT ${BIN_FILENAME}

    COMMAND ${CMAKE_VXX_COMPILER}
    -l
    -t ${XCL_TARGET} 
    -o ${BIN_FILENAME} 
    --platform ${XCL_PLATFORM} 
    --config ${CFG_FILENAME}
    --config ${VIVADO_OPT_FILENAME}
    --kernel_frequency ${KERNEL_FREQ} 
    --save-temps
    --temp_dir ${BUILD_TMP}
    ${VXX_FLAGS}
    ${OBJ_FILENAMES}

    COMMENT "Linking target: ${BIN_FILENAME}"

    DEPENDS ${OBJ_FILENAMES}
    DEPENDS ${CFG_FILENAME}
    )

add_custom_command(
    OUTPUT ${FINAL_BIN_FILENAME}

    COMMAND ${XCLBINUTIL}
    -i ${BIN_FILENAME}
    -o ${FINAL_BIN_FILENAME}
    --key-value USER:CSRE_MAX_CHAR_NUM:${CSRE_MAX_CHAR_NUM}
    --key-value USER:CSRE_MAX_STATE_NUM:${CSRE_MAX_STATE_NUM}
    --key-value USER:CSRE_MAX_CHAR_PER_TOKEN:${CSRE_MAX_CHAR_PER_TOKEN}
    --key-value USER:CSRE_MAX_TOKEN_NUM:${CSRE_MAX_TOKEN_NUM}
    --key-value USER:CSRE_MAX_COUNT_NUM:${CSRE_MAX_COUNT_NUM}
    --key-value USER:CSRE_NUM_PIPELINE:${CSRE_NUM_PIPELINE}
    --key-value USER:CSRE_RESULT_BURST_LENGTH:${CSRE_RESULT_BURST_LENGTH}
    --key-value USER:CSRE_NUM_PU:${CSRE_NUM_PU}
    --key-value USER:CSRE_NUM_PATTERN_REG_L1:${CSRE_NUM_PATTERN_REG_L1}

    DEPENDS ${BIN_FILENAME}
    )

if (${XCL_TARGET} STREQUAL "hw_emu")
    # Generate emulation config
    set(EMCONFIG ${CMAKE_CURRENT_BINARY_DIR}/emconfig.json)
    add_custom_command(
        OUTPUT ${EMCONFIG}

        COMMAND ${EMCONFIGUTIL}
        --platform ${XCL_PLATFORM}
        --od ${CMAKE_CURRENT_BINARY_DIR}

        COMMENT "Generating emconfig file: ${EMCONFIG}"
        )

    add_custom_target(sim
        DEPENDS ${EMCONFIG}
        DEPENDS ${FINAL_BIN_FILENAME}
        )

    install(FILES ${EMCONFIG} DESTINATION ${INSTALL_DIR})
else()
    add_custom_target(image
        DEPENDS ${FINAL_BIN_FILENAME}
        )

endif()

add_subdirectory(string-match/utils)

install(FILES ${FINAL_BIN_FILENAME} DESTINATION ${INSTALL_DIR} OPTIONAL)
install(TARGETS recomp DESTINATION ${INSTALL_DIR})
install(TARGETS recomp DESTINATION ${INSTALL_DIR}/csre-comp/lib)
install(TARGETS csre_check_regex DESTINATION ${INSTALL_DIR})
