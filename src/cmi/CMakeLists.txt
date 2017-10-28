cmake_minimum_required(VERSION 2.6)

project(cmi${LIB_SUFFIX})


include_directories( ../../include  ${INCLUDE_DIR})

file(GLOB HEADERS
    "../../include/cmi/*.h"
)

set( SRCS
    CAN.cpp
    CanMessage.cpp
    CAN_Interface.cpp
    CMI.cpp
    CO301_interface.cpp
    EventDispatcher.cpp
    MAL_Interface.cpp
    MAL_CANOpen402.cpp
    ObjectDatabase.cpp
    ObjectDictionary.cpp
    globals.cpp
)

if(NOT MSVC)
    set(WHOLE_ON   "-Wl,--whole-archive")
    set(WHOLE_OFF  "-Wl,--no-whole-archive")
endif()

set(PROJ_DEPENDENCIES
        ${WHOLE_ON}
        cmi_os${LIB_SUFFIX}_static
        ${WHOLE_OFF}
        pthread
        tinyxml2
        abseil_cpp
)

if( NOT WIN32)
    set(PROJ_DEPENDENCIES ${PROJ_DEPENDENCIES}
        rt
        dl )
else()
    set(PROJ_DEPENDENCIES ${PROJ_DEPENDENCIES}
        ws2_32 )
endif()

if( NOT ARM_TARGET AND TA_GUID )
    message("Compiling with TurgoActivate")

        set( PROJ_DEPENDENCIES ${PROJ_DEPENDENCIES}
              "-Wl,--whole-archive"
              TurboActivate${BITS}
              "-Wl,--no-whole-archive"
            )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTA_GUID=\"${TA_GUID}\"" )
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/lib/linux/libTurboActivate-${BITS}.so       DESTINATION  lib )
endif()

add_library( ${PROJECT_NAME} SHARED ${SRCS} ${HEADERS} )
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${PROJ_DEPENDENCIES} )

set_target_properties(${PROJECT_NAME} PROPERTIES   SOVERSION 1.6.0 )


INSTALL(TARGETS ${PROJECT_NAME}        DESTINATION  lib )


