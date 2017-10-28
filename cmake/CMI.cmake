
SET(CANMOVEIT 1)


 ############################### Compilation type #############################################
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
   message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
   set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
   # Set the possible values of build type for cmake-gui
   set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
 endif()
 
 message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

 ############################### Output directories #############################################

 # First for the generic no-config case (e.g. with mingw)
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
  set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib )

 # Second, for multi-config builds (e.g. msvc)

  foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
      string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin )
      set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib )
  endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

 ############################### 32 or 64 bits? #############################################
if( ARM_TARGET )

    MESSAGE("Arm platform: 32 bits" )
    SET( BITS "32" )

else()

    if( CMAKE_SIZEOF_VOID_P EQUAL 8 OR FORCE_64BIT AND NOT FORCE_32BIT)
        message(STATUS "64 bits compiler detected" )
        set( BITS "64" )

    elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 OR FORCE_32BIT AND NOT FORCE_64BIT)

        message(STATUS "32 bits compiler detected" )
        set( BITS "32" )

    endif( )

    if(NOT WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m${BITS}")
    endif()

endif ()

######################## Suffix of the libraries  ############################

if(WIN32)

    set(PLATFORM "windows")
    message(STATUS "Compiling for Windows")

    if (MINGW )
        SET( COMPILER "-mgw48" )
    elseif( MSVC )
        SET( COMPILER "-vc120" )
    endif()

    IF(CMAKE_BUILD_TYPE MATCHES DEBUG)
        set( DEBUG_SUFFIX "-gd" )
    else()
        set( DEBUG_SUFFIX )
    ENDIF(CMAKE_BUILD_TYPE MATCHES DEBUG)

    set( LIB_SUFFIX ${COMPILER}${DEBUG_SUFFIX} )
    set( BOOST_LIB_SUFFIX ${COMPILER}-mt${DEBUG_SUFFIX}-1_54 )

elseif(UNIX)

    set(PLATFORM "linux")
    message(STATUS "Compiling for Linux")

    set( COMPILER "-gcc" )

    if( ARM_TARGET)

        set( LIB_SUFFIX -gcc-arm )
        set(BOOST_LIB_SUFFIX ${LIB_SUFFIX})

    else( ARM_TARGET )

        set( LIB_SUFFIX -gcc-${BITS} )
        set(BOOST_LIB_SUFFIX )

    endif( ARM_TARGET)

else()
        message( FATAL_ERROR "Platform not supported")
endif()


######################## Compilation flags and dependencies ##########################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCMI_LIB_SUFFIX=${LIB_SUFFIX} -DBOOST_ALL_DYN_LINK")

set (CMI_DEPENDENCIES ${CMI_DEPENDENCIES}
            cmi${LIB_SUFFIX}
            boost_atomic${BOOST_LIB_SUFFIX}
            boost_thread${BOOST_LIB_SUFFIX}
            boost_chrono${BOOST_LIB_SUFFIX}
            boost_system${BOOST_LIB_SUFFIX}
            boost_filesystem${BOOST_LIB_SUFFIX}
            boost_log_setup${BOOST_LIB_SUFFIX}
            boost_log${BOOST_LIB_SUFFIX}
            )

if(WIN32)

    set (CMI_DEPENDENCIES ${CMI_DEPENDENCIES}
            pthread
            ws2_32
    )
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

elseif(UNIX)

    set (CMI_DEPENDENCIES ${CMI_DEPENDENCIES}
            pthread
            dl
            rt
    )
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -DLINUX -fPIC")

else()
        message( FATAL_ERROR "Platform not supported")
endif()

############################################################################
