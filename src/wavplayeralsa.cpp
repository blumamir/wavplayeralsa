
#include "single_file_player.h"
#include "position_reporter.h"
#include "generated/player_command.pb.h"
#include <iostream>
#include <sndfile.h>

#include <zmq.hpp>
#include <string>
#include <iostream>
#include <sstream>
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

	wavplayeralsa::SingleFilePlayer *player = new wavplayeralsa::SingleFilePlayer();
	player->setFileToPlay(filename);
	try {
		player->initialize();		
	}
	catch(const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "goodbey" << std::endl;
		return -1;
	}
	player->startPlay(0);
	wavplayeralsa::PositionReporter pr;
	while(true) {

		zmq::message_t request;
		bool gotMsg = socket.recv(&request);
		if(!gotMsg) {
			if(player != NULL) {
				unsigned int positionMs = player->getPositionInMs();
				pr.sendNewPosition(positionMs, player->getFileToPlay());			
			}
		}
		else {

			PlayerCommandMsg reqMsg;
			std::string msgStr(static_cast<char*>(request.data()), request.size());
			reqMsg.ParseFromString(msgStr);
			std::cout << "Got player request message " << std::endl << reqMsg.DebugString() << std::endl;

			bool reqStatus = true;
			std::stringstream reqStatusDesc;
			if(reqMsg.has_stop_play()) {
				if(player != NULL) {
					player->stop();
					reqStatusDesc << "current song '" << player->getFileToPlay() << "' stopped playing";
					reqStatus = true;
				}
				else {
					reqStatusDesc << "asked to stop song, but there is no song loaded in the player";
					reqStatus = true;
				}
			}

			else if(reqMsg.has_new_song_request()) {

				const PlayerCommandMsg::NewSongRequest &newSongReq = reqMsg.new_song_request();
				const std::string newSongToPlay = newSongReq.song_name();
				const unsigned int newPositionMs = newSongReq.position_in_ms(); // will default to 0 if not set

				if(player != NULL && newSongToPlay == player->getFileToPlay()) {
					reqStatusDesc << "changing the position of the current song '" << player->getFileToPlay() << "'. new position in ms is: " << newPositionMs << std::endl;
					reqStatus = true;
				}
				else {
					// so the request asks to change the song.
					// we will first delete the previous song, then try to load the new one.
					// in case we fail, we will be left with no song at all which is fine

					if(player != NULL) {
						player->stop();
						delete player;
					}

					player = new wavplayeralsa::SingleFilePlayer();
					player->setFileToPlay(newSongReq.song_name());
					try {
						player->initialize();		
						reqStatusDesc << "song successfully changed to '" << newSongToPlay << "'. " <<
								"new song will start playing at position " << newPositionMs << " ms";
						reqStatus = true;
					}
					catch(const std::runtime_error &e) {
						delete player;
						player = NULL;
						reqStatusDesc << "loading new song '" << newSongToPlay << "' failed. currently no song is loaded in the player and it is not playing. " <<
							"reason for failure: '" << e.what() << "'";
						reqStatus = false;
					}
				}

				if(player != NULL) {
					player->startPlay(newSongReq.position_in_ms());
				}
			}

			PlayerCommandReplyMsg replyProtoMsg;
			replyProtoMsg.mutable_req_identifier()->CopyFrom(reqMsg.req_identifier());
			replyProtoMsg.set_req_status(reqStatus);
			replyProtoMsg.set_req_status_desc(reqStatusDesc.str());

			std::string replayProtoSerialized;
			replyProtoMsg.SerializeToString(&replayProtoSerialized);
			zmq::message_t reply(replayProtoSerialized.size());
			memcpy(reply.data(), replayProtoSerialized.c_str(), replayProtoSerialized.size());
			socket.send(reply);

			std::cout << "sent reply: " << std::endl << replyProtoMsg.DebugString();
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	return 0;
}


