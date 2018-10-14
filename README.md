# wavplayeralsa
## Installation process (tested on Raspbian and Ubuntu)
0. Install Raspbian https://www.raspberrypi.org/documentation/installation/installing-images/
1. Install protobuf 2.6.1
'''
  wget https://github.com/protocolbuffers/protobuf/releases/download/v2.6.1/protobuf-2.6.1.tar.gz
  tar -xvzf protobuf-2.6.1.tar.gz
  cd protobuf-2.6.1/
  ./configure
  make
  make check
  sudo make install
  sudo ldconfig
'''   
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
  
5. Install cxxopts lib

  wget https://github.com/jarro2783/cxxopts/archive/v2.1.1.tar.gz
  
  tar xzvf v2.1.1.tar.gz
  
  cd cxxopts-2.1.1
  
  cmake .
  
  make -j4
  
  sudo make -j4 install
  
6. Clone the wavplayeralsa project and compile it

  git clone https://github.com/BlumAmir/wavplayeralsa.git
  
  cd wavplayeralsa/
  
  cmake .
  
  make
  
7. Create a configuration file for the player

## Player description
1. The player is a wav files audio player intended for accurate position tracking. 
2. Control over the player is done over a Zero MQ socket, selecting the currently playing file and starting position. 
3. The player publishes the currently playing file list and accurate current position in milliseconds over UDP broadcast (to minimize network latency)

## Usage
The executable supports a few commandline arguments, for a list of available options use ./wavplayeralsa -h

To play a specific file use ./wavplayeralsa -f <filename>.wav

## Control interface
To control and query the player state a Zero MQ socket can be opened to port 2100 (default), the port can be changed using the commandline parameter --cmd_ifc_port <port>
  
The socket should be of type REQ/REP

The message content is described in the file player_command.proto, command to the player should be of type PlayerCommandMsg, afterwards the player will always respond with a PlayerCommandReplyMsg

If a command is not specified the reply message still contains the current song name and an indication if a song is playing (can be useful for polling the player)

## Position report interface
The player will publish a message containing the current song and current position within the song every 5 ms (default), the publishing rate can be changed using the --position_report_rate <rate in ms> commandline parameter
  
The message is broadcasted over the local interface on port 2001 (default), port can be changed using the --position_report_port <port> commandline parameter
  
The content of the message is a protobuf message specified in the position_report.proto file

The player currently only support file lists of size 0 (no song is playing) or 1 (currently playing song)
