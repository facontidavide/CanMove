# this one is important
INCLUDE(CMakeForceCompiler)

SET(CMAKE_SYSTEM_NAME Linux)

SET(CERBERUS true)


# IMPORTANT: please change the following path
SET( COMPILER_PATH   /opt/OSELAS.Toolchain-2013.12.2/arm-cortexa8-linux-gnueabihf/gcc-4.8.3-glibc-2.18-binutils-2.24-kernel-3.12-sanitized)
SET( TOOLCHAIN_PATH  /opt/cmi)

# specify the cross compiler
SET(CMAKE_C_COMPILER    ${COMPILER_PATH}/bin/arm-cortexa8-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER  ${COMPILER_PATH}/bin/arm-cortexa8-linux-gnueabihf-g++)


SET(CMAKE_C_COMPILER_FORCED TRUE)
SET(CMAKE_CXX_COMPILER_FORCED TRUE)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH ${COMPILER_PATH}/ )

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
