
# How to compile the player for Raspberry Pi from linux
The mqtt library is heavily-templated, and takes a lot of memory to compile (>1G) which does not work on RPI.
To compile the player for RPI, use cross compiltion from linux machine with enough memory:

## install

### install cross compiler package for armhf (beaglebone / raspberry-pi 32 bit)
`sudo apt install crossbuild-essential-armhf`

### install sshfs so we can mount the pi's filesystem to the PC
`sudo apt install sshfs`

## mount rpi fs to linux

SSHFS here will mount the raspberry pi (its root - /) to the local filesystem

### activate the mount
`sudo sshfs ${RPI_USERNAME}@${RPI_IP}:/ $MOUNT_POINT`
example: `sudo sshfs pi@10.0.0.200:/ /mnt/rpi`

if for some reason there's an issue with the mount - do: `ls -l /mnt` if you see a lot of '?????????' on /mnt/rpi then run: `fusermount -u /mnt/rpi`
and run the sshfs command again.

## compile the player on linux with RPI mount

1. clone the repository to a directory on RPI file system (using the mount, or directly).
2. then `cd` to the project directory on the mount fs, for example: `cd /mnt/rpi/home/pi/dev/wavplayeralsa`
3. create the cmake build directory: `mkdir build && cd build`
4. run camke with relevant toolchain camke file: `cmake -DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE ..` where $TOOLCHAIN_FILE should point to the `toolchain-armv7l.cmake` file from cross-compile directory in the project repo. note that this file assumes rpi mounting point is at `/mnt/rpi`.
5. `make -j2` the generated project.
6. the executable is now in the build directory on the rpi

