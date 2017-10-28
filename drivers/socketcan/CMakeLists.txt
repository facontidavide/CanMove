cmake_minimum_required(VERSION 2.6)

project(driver_socket${LIB_SUFFIX})

include_directories( ../include  ${INCLUDE_DIR} )


add_library( ${PROJECT_NAME} SHARED can_driver_socket.cpp )

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION lib )

if(CMAKE_HOST_UNIX)
    if( (CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT FORCE_32BIT) OR
        (CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT FORCE_64BIT) )
    INSTALL(CODE "
              EXECUTE_PROCESS (COMMAND ln -sf  ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME} /usr/lib/libdriver_socket.so )
    ")
    endif( )
endif(CMAKE_HOST_UNIX)
