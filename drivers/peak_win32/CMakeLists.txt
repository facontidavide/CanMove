cmake_minimum_required(VERSION 2.6)

include_directories( ../include  ${INCLUDE_DIR} ./ )

set(CMAKE_CXX_FLAGS "-DPOCO_OS_FAMILY_WINDOWS -DDLL_EXPORTS -DWIN32")


add_library( driver_peak_win32 SHARED can_driver_peak_win32.cpp )

TARGET_LINK_LIBRARIES(driver_peak_win32 ${CMAKE_CURRENT_SOURCE_DIR}/VC_LIB/PCANBasic.lib)

INSTALL(TARGETS driver_peak_win32 DESTINATION ${CMI_CORE_PATH}/lib )
