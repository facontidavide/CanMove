cmake_minimum_required(VERSION 2.6)

project( cmi_os${LIB_SUFFIX}_static )

include_directories( ../../include 
                     ${INCLUDE_DIR})


file(GLOB SRCS
    "*.cpp"
    "../3rdparty/*.cpp"
  #  "../../include/OS/*.h"
  #   "../../include/rdparty/*.h"
)

if(WIN32)
        file(GLOB TO_BE_REMOVED "*_UNIX.cpp" "*_POSIX.cpp")
else()
        file(GLOB TO_BE_REMOVED "*_WIN32.cpp" )
endif()

list(REMOVE_ITEM SRCS ${TO_BE_REMOVED} )


add_library( ${PROJECT_NAME} ${SRCS} )
target_link_libraries(  ${PROJECT_NAME} dl pthread boost_system)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION lib )
