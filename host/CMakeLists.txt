set(XILINX_XRT $ENV{XILINX_XRT})
set(XRT_CORE_LIBRARY xrt_core)

# Link to emulation specific lib if running in XCL_EMULATION_MODE
if (${XCL_TARGET} STREQUAL "hw_emu")
    string(REPLACE "_" "" XCL_EMU_SUFFIX ${XCL_TARGET})
    set(XRT_CORE_LIBRARY xrt_${XCL_EMU_SUFFIX})
    message("Emulation mode enabled: XRT_CORE_LIB=${XRT_CORE_LIBRARY}")
endif()

# Find required libraries
find_package(Boost 1.53 REQUIRED COMPONENTS thread chrono system)

message("boost lib: ${Boost_LIBRARIES}")
if (NOT Boost_FOUND)
    message(FATAL_ERROR "libboost not found!")
endif()

find_library(xrt_core_LIBRARY
    NAMES ${XRT_CORE_LIBRARY}
    HINTS "${XILINX_XRT}/lib")
if (NOT xrt_core_LIBRARY)
    message(FATAL_ERROR "xrt_core not found! Check if XILINX_XRT points to the correct XRT install base!")
endif()

find_library(xrt_coreutil_LIBRARY
    NAMES xrt_coreutil
    HINTS "${XILINX_XRT}/lib")
if (NOT xrt_coreutil_LIBRARY)
    message(FATAL_ERROR "xrt_coreutil not found! Check if XILINX_XRT points to the correct XRT install base!")
endif()

find_library(xrt_xilinxopencl_LIBRARY
    NAMES xilinxopencl
    HINTS "${XILINX_XRT}/lib")
if (NOT xrt_xilinxopencl_LIBRARY)
    message(FATAL_ERROR "xilinxopencl not found! Check if XILINX_XRT points to the correct XRT install base!")
endif()

find_library(uuid_LIBRARY
    NAMES uuid)
if (NOT uuid_LIBRARY)
    message(FATAL_ERROR "libuuid not found!")
endif()

include_directories(${XILINX_XRT}/include)

file(GLOB_RECURSE RUNTIME_COMMON "runtime/common/*.cpp")
file(GLOB_RECURSE RUNTIME_OCL    "runtime/ocl/*.cpp")
file(GLOB_RECURSE CORE_SRC       "core/*.c")
file(GLOB_RECURSE REF_SRC        "ref/*.cpp")
file(GLOB_RECURSE XCL2_SRC       "xcl2/*.cpp")

add_library(csre-ocl SHARED ${CORE_SRC} ${REF_SRC} ${RUNTIME_COMMON} ${RUNTIME_OCL} ${XCL2_SRC})
target_link_libraries(csre-ocl PRIVATE recomp ${xrt_xilinxopencl_LIBRARY} uuid boost_system boost_chrono)
set_property(TARGET csre-ocl PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)
target_include_directories(csre-ocl PRIVATE ${PROJ_ROOT}/host/include)

install(TARGETS csre-ocl DESTINATION ${INSTALL_DIR})

add_subdirectory(app/single-thread)
add_subdirectory(app/multi-thread)
add_subdirectory(app/csre-comp-example)

add_custom_target(sw ALL
        DEPENDS csre-st
        DEPENDS csre-st-ocl
        DEPENDS csre-mt
        )
