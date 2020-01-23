# this one is important
SET(CMAKE_SYSTEM_NAME Linux)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /usr/bin/arm-linux-gnueabihf-gcc-7)
SET(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++-7)

# where is the target environment
SET(CMAKE_FIND_ROOT_PATH  /mnt/rpi)
SET(CMAKE_SYSROOT /mnt/rpi)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

