
#include "single_file_player.h"
#include "position_reporter.h"
#include "generated/player_command.pb.h"
#include <iostream>
#include <sndfile.h>

#include <zmq.hpp>
#include <string>
#include <iostream>
#include <unistd.h>


int main(int argc, char *argv[]) {

	if (argc <= 1) {
		std::cout << "Usage: " << argv[0] << " file.wav" << std::endl;
		return -1;
	}

	zmq::context_t context(1);
	zmq::socket_t socket(context, ZMQ_REP);
	int rcvtimeo = 5;
	socket.setsockopt(ZMQ_RCVTIMEO, &rcvtimeo, sizeof(int));
	socket.bind("tcp://*:5555");

	const char *filename = argv[1];

	wavplayeralsa::SingleFilePlayer player;
	player.setFileToPlay(filename);
	try {
		player.initialize();		
	}
	catch(const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "goodbey" << std::endl;
		return -1;
	}
	player.startPlay(0);

	wavplayeralsa::PositionReporter pr;
	while(true) {

		zmq::message_t request;
		int rc = socket.recv(&request);
		if(rc == 0) {
			unsigned int positionMs = player.getPositionInMs();
			pr.sendNewPosition(positionMs, filename);			
		}
		else {
			PlayerCommandMsg reqMsg;
			std::string msgStr(static_cast<char*>(request.data()), request.size());
			reqMsg.ParseFromString(msgStr);
			if(reqMsg.has_stop_play()) {
				player.stop();
			}
			if(reqMsg.has_new_song_request()) {
				player.startPlay(reqMsg.new_song_request().position_in_ms());
			}
			std::cout << "Got message " << std::endl << reqMsg.DebugString() << std::endl;
			zmq::message_t reply(5);
			memcpy(reply.data(), "OK", 5);
			socket.send(reply);
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	return 0;
}


