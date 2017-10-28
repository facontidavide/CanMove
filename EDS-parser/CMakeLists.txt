cmake_minimum_required(VERSION 2.6)

include_directories( ../include  ${INCLUDE_DIR} )

if(WIN32)
    set( DEPENDENCIES cmi_os cmi )
else()
    set( DEPENDENCIES
        pthread
        CMI_chrono
        CMI_system
        CMI_thread
        cmi_os
        TurboActivate
        cmi
        )
endif()


add_executable( eds-parser   eds-parser.cpp )
TARGET_LINK_LIBRARIES( eds-parser ${DEPENDENCIES})

