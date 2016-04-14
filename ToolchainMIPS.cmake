include(CMakeForceCompiler)

# this one is important
set(CMAKE_SYSTEM_NAME Linux)
#this one not so much
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_SYSTEM_PROCESSOR mips)

#
# stbgcc toolchain
#
set(STBGCC_PATH /opt/toolchains/stbgcc-4.8-1.2)

#
# specify the cross compiler
#
set(CMAKE_C_COMPILER   ${STBGCC_PATH}/bin-ccache/mipsel-linux-gcc)
set(CMAKE_CXX_COMPILER ${STBGCC_PATH}/bin-ccache/mipsel-linux-g++)

# where is the target environment
set(CMAKE_FIND_ROOT_PATH ${STBGCC_PATH}/target ${EXTRA_ROOT_PATH})

# search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

