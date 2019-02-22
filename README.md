# wavplayeralsa
## Installation process (tested on Raspbian and Ubuntu)
0. Install Raspbian https://www.raspberrypi.org/documentation/installation/installing-images/
1. Install sndfile and libsound libs
```
  sudo apt-get install libsndfile1-dev libasound2-dev
```  
2. Clone the wavplayeralsa project and compile it
```
  git clone https://github.com/BlumAmir/wavplayeralsa.git
  cd wavplayeralsa/
  cmake .
  make
```
3. Create a configuration file for the player

## Player description
1. The player is a wav files audio player intended for accurate position tracking. 
2. Control over the player is done over HTTP, selecting the currently playing file and starting position. 
3. The player publishes the currently playing file and accurate start time position in milliseconds since epoch over web socket. Client can calculate precise offset in song by using it's local clock and the start time information.

## Usage
The executable supports a few commandline arguments, for a list of available options use ./wavplayeralsa -h

To play a specific file use ./wavplayeralsa -f <filename>.wav

TODO
## Control interface

TODO
## Position report interface
