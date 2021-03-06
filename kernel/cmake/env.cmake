cmake_minimum_required(VERSION 3.13.0)
# Global Variables
set(SEP "/")
set(PATH_SEP ":")
set(CONDA_PREFIX "")
set(CMAKE_PREFIX_PATH $ENV{CMAKE_PREFIX_PATH})

## Xilinx
set(XILINX_XRT $ENV{XILINX_XRT})
set(XILINX_VITIS $ENV{XILINX_VITIS})
set(XILINX_VIVADO $ENV{XILINX_VIVADO})
set(XCL_KERNEL $ENV{XCL_KERNEL})
set(XCL_EMULATION_MODE $ENV{XCL_EMULATION_MODE})

## CMake
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib ${XILINX_XRT}/lib)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

## C++
set(CMAKE_CXX_STANDARD 17)
