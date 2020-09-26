# wavplayeralsa
## Installation process (tested on Raspbian and Ubuntu)
0. Install Raspbian https://www.raspberrypi.org/documentation/installation/installing-images/
1. Install boost, sndfile and libsound libs
```
  sudo apt-get install libsndfile1-dev libasound2-dev libboost-all-dev uuid-dev
```  
2. Clone the wavplayeralsa project and compile it
```
  git clone https://github.com/BlumAmir/wavplayeralsa.git
  cd wavplayeralsa/
  mkdir bin
  cd bin
  cmake ..
  make
```
3. Create a configuration file for the player

## Player description
1. The player is a wav files audio player intended for accurate position tracking. 
2. Control over the player is done over HTTP, selecting the currently playing file and starting position. 
3. The player publishes the currently playing file and accurate start time position in milliseconds since epoch over web socket. Client can calculate precise offset in song by using it's local clock and the start time information.

## Usage
The executable supports a few commandline arguments, for a list of available options use ./wavplayeralsa -h

To play a specific file use `./wavplayeralsa -f <file_name>.wav`


## logging
Player write log messages to console.
By setting `log_dir` command line argument to an existing directory, player will create a unique log file for each run.


## Control interface
Controling the player is done via HTTP interface.
Player's command line option 'http_listen_port' is used to set the port on which the player listens for control commands and queries.

To play an audio file, send a json to uri http://PLAYE_IP:HTTP_LISTEN_PORT/api/current-song in the following format:
`{ "file_id": "path/from/wav/dir/<file_name>.wav", "start_offset_ms":0 }`
example with curl:
```
curl -X PUT -H "Content-Type: application/json" -d "{\"file_id\": \"<file_name>.wav\", \"start_offset_ms\":0}" "http://127.0.0.1:8080/api/current-song"
```
`start_offset_ms` can be negative, in which case song will start to play in the future.

To stop an audio file which is currently playing, send a json to uri http://YOUR_IP:HTTP_LISTEN_PORT/api/current-song with empty or missing 'file_id':
`{ "file_id": "" }` or `{}`
example with curl :
```
curl -X PUT -H "Content-Type: application/json" -d "{}" "http://127.0.0.1:8080/api/current-song"
```


## Position report interface
Player's command line option 'ws_listen_port' is used to set the port on which the player listens for web sockets client who wish to receive push notifications on events:

When a new audio file is played, or when the current audio position is changed externally:
`{"file_id":"<file_name>.wav","song_is_playing":true,"speed":1.0,"start_time_millis_since_epoch":1551335294511}`

When a stop is performed via control interface, or when audio reach end of file:
`{"song_is_playing":false}`

`start_time_millis_since_epoch` is the audio's file start time (position 0) in milliseconds, since UNIX Epoch time (00:00:00 Thursday, 1 January 1970, UTC).
Client can calculate the file's audio position at any givin time, using it's local clock, which should be synchronized to the player's clock.
This enable clients to act upon precise and continuous audio position, which does not dependent on network latency and update rate.
Any offset in clock synchronization (between client's and player's os) will be carried to audio position calculation, thus user should assure such offset is minimal (using NTP for example, or running client on same machine as player).

## Docker
You can run the player as a docker.

Build: `docker build -t wavplayeralsa .`

Run: `docker run -p 8080:80 -v ${MusicDirectory}:/wav_files --device /dev/snd --name wavplayeralsa wavplayeralsa`