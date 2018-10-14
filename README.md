# wavplayeralsa
## Installation process (tested on Raspbian and Ubuntu)
0. Install Raspbian https://www.raspberrypi.org/documentation/installation/installing-images/
1. Install protobuf 2.6.1
  wget https://github.com/protocolbuffers/protobuf/releases/download/v2.6.1/protobuf-2.6.1.tar.gz
  tar -xvzf protobuf-2.6.1.tar.gz
   cd protobuf-2.6.1/
   ./configure
   make
   make check
   sudo make install
   sudo ldconfig
2. Install zmq
  sudo apt-get install libzmq3-dev
3. Install cppzmq using cmake
  wget https://github.com/zeromq/cppzmq/archive/v4.3.0.tar.gz
  tar xzvf v4.3.0.tar.gz
  cd cppzmq-4.3.0/
  mkdir build
  cd build/
  sudo apt-get install cmake
  cmake ..
  sudo make -j4 install
  cd ../..
4. Install sndfile and libsound libs
  sudo apt-get install libsndfile1-dev libasound2-dev
5. Clone the wavplayeralsa project and compile it
  git clone https://github.com/BlumAmir/wavplayeralsa.git
  cd wavplayeralsa/
  cmake .
  make
6. Create a configuration file for the player
