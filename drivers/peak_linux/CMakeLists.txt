cmake_minimum_required(VERSION 2.6)

project(driver_peak_linux${LIB_SUFFIX})

include_directories( ../include  ${INCLUDE_DIR} ./ )

set(CMAKE_C_FLAGS  "-fPIC -Wall -gdwarf-3 ")
set(CMAKE_CXX_FLAGS  "-fPIC -Wall -gdwarf-3 ")

add_library( ${PROJECT_NAME} SHARED can_driver_peak_linux.cpp libpcan.c)


INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${CMI_CORE_PATH}/lib )
