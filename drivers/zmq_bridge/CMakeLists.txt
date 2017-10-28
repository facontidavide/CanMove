cmake_minimum_required(VERSION 2.6)

IF(NOT DEFINED CANMOVEIT)
    include(/etc/cmi/CMI.cmake)
endif()

include_directories( ../include  ${INCLUDE_DIR} )


add_library( driver_zmq_bridge${LIB_SUFFIX}    SHARED can_driver_zmq.cpp )
TARGET_LINK_LIBRARIES( driver_zmq_bridge${LIB_SUFFIX} 
        "-Wl,--whole-archive"
        zmq
         "-Wl,--no-whole-archive"
        )
INSTALL(TARGETS driver_zmq_bridge${LIB_SUFFIX} DESTINATION ${CMI_CORE_PATH}/lib )


add_executable(zmq2can_server zmq2can_server.cpp)
TARGET_LINK_LIBRARIES( zmq2can_server ${CMI_DEPENDENCIES} zmq${LIB_SUFFIX})

