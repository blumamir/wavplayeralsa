# wavplayeralsa


Installation process for the player on Linux Ubuntu, tested on 18.04
1. Install compiler: sudo apt install build-essential
2. Install Git: sudo apt install git
3. Clone the project
4. Install CMake
5. Install zmq
  ./configure
  make
  make install
6. sudo apt-get install libzmq3-dev
7. Install zmqcpp using cmake
8. Install Protobuf 2.6.1
  wget https://github.com/protocolbuffers/protobuf/releases/download/v2.6.1/protobuf-2.6.1.tar.gz
  build protobuf according to instructions
  https://github.com/protocolbuffers/protobuf/blob/master/src/README.md
9. Install sndfile and libsound libs
  sudo apt-get install libsndfile1-dev libasound2-dev
10. Install wavplayeralsa
