cmake_minimum_required(VERSION 2.6)

project(CanMoveIt)

# First for the generic no-config case (e.g. with mingw)
 set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
 set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

# Second, for multi-config builds (e.g. msvc)

 foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
     string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
     set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin )
     set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib )
 endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
############################################################################

include(${CMAKE_SOURCE_DIR}/cmake/CMI.cmake)
add_definitions( -std=c++11 )
##################################################################

SET(TOOLCHAIN_PATH "/opt/cmi")

IF(NOT DEFINED TOOLCHAIN_PATH)
      message( FATAL_ERROR "Please define TOOLCHAIN_PATH using the location where CMI/Core is located")
ENDIF(NOT DEFINED TOOLCHAIN_PATH)

if(WIN32)
    SET(PLATFORM "windows")
    message("Compiling for Windows")

elseif(UNIX)
    SET(PLATFORM "linux")
    message("Compiling for Linux")
else()
    message( FATAL_ERROR "Platform not supported")
endif()

############################################################################

link_directories( ${CMAKE_SOURCE_DIR}/lib/linux )

include_directories( ${CMAKE_SOURCE_DIR}/include
                     ${CMAKE_SOURCE_DIR}/3rdparty
                     ${CMAKE_SOURCE_DIR}/3rdparty/absl/cctz/include )

############################################################################
add_subdirectory(3rdparty/tinyxml2)
add_subdirectory(3rdparty/absl)
add_subdirectory(src/OS)
add_subdirectory(src/cmi)
#add_subdirectory(EDS-parser)
#add_subdirectory(dictionaries)
add_subdirectory(tests)

if(WIN32)
    add_subdirectory(drivers/peak_win32)
else()
    add_subdirectory(drivers/socketcan)
  #  add_subdirectory(drivers/peak_linux)
  #  add_subdirectory(drivers/zmq_bridge )
endif()


INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/include    DESTINATION ${TOOLCHAIN_PATH}  )

FILE(GLOB etcfiles "${CMAKE_SOURCE_DIR}/etc/*")
INSTALL(FILES ${etcfiles} DESTINATION /etc/cmi)

FILE(GLOB cmakefiles "${CMAKE_SOURCE_DIR}/cmake/*")
INSTALL(FILES ${cmakefiles} DESTINATION ${TOOLCHAIN_PATH}/cmake)

INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/src/OS     DESTINATION ${TOOLCHAIN_PATH}/src  )


