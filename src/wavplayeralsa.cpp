
#include "single_file_player.h"
#include "position_reporter.h"
#include "generated/player_command.pb.h"
#include <iostream>
#include <sndfile.h>
#include <cxxopts.hpp>
#include <zmq.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <linux/limits.h>


int main(int argc, char *argv[]) {

	// current directory
	char cwdCharArr[PATH_MAX];
	getcwd(cwdCharArr, sizeof(cwdCharArr));

	wavplayeralsa::PositionReporter pr;
	wavplayeralsa::SingleFilePlayer *player = NULL;


	cxxopts::Options options("wavplayeralsa", "wav files player with accurate position in audio tracking.");
	options.add_options()
		("f,initial_file", "file which will be played on run", cxxopts::value<std::string>())
		("d,wav_dir", "the directory in which wav files are located", cxxopts::value<std::string>()->default_value(cwdCharArr))
		("position_report_port", "port on which live position messages are published over broadcast", cxxopts::value<uint16_t>()->default_value("2001"))
		("position_report_rate", "the rate for position report messages in ms", cxxopts::value<uint32_t>()->default_value("5"))
		("cmd_ifc_port", "port to listen for player commands over network", cxxopts::value<uint16_t>()->default_value("2100"))
		("h, help", "print help")
		;

	int rcvtimeo = 5;
	uint16_t cmdIfcPort = 2100;
	std::string wavDir;

	try {

		 auto optsresult = options.parse(argc, argv);

		 if(optsresult.count("help")) {
		 	std::cout << options.help({""}) << std::endl;
		 	return 0;
		 }

		 // position reporter stuff
		 pr.initialize(optsresult["position_report_port"].as<uint16_t>());

		 // player stuff
		 wavDir = optsresult["wav_dir"].as<std::string>();
		 if(optsresult.count("initial_file")) {
		 	player = new wavplayeralsa::SingleFilePlayer();
		 	player->initialize(wavDir, optsresult["initial_file"].as<std::string>());
		 }

		 // commands
		 rcvtimeo = optsresult["position_report_rate"].as<uint32_t>();
		 cmdIfcPort = optsresult["cmd_ifc_port"].as<uint16_t>();

	}
	catch(const cxxopts::OptionException &e) {
		std::cout << "Invalid command line options: '" << e.what() << "'" << std::endl;
		return -1;
	}
	catch(const std::runtime_error &e) {
		std::cout << "general error: '" << e.what() << "'" << std::endl;
		return -1;
	}

	zmq::context_t context(1);
	zmq::socket_t socket(context, ZMQ_REP);
	socket.setsockopt(ZMQ_RCVTIMEO, &rcvtimeo, sizeof(int));

	std::stringstream bindStr;
	bindStr << "tcp://*:" << cmdIfcPort;
	socket.bind(bindStr.str().c_str());

	if(player != NULL) {
		player->startPlay(0);
	}

	uint32_t positionCookie = 1;
	while(true) {

		// TODO: this code was just for POC. need to arrange it and make it preety
		zmq::message_t request;
		bool gotMsg = socket.recv(&request);
		if(!gotMsg) {
			if(player != NULL) {
				if(player->isPlaying()) {
					unsigned int positionMs = player->getPositionInMs();
					pr.sendNewPosition(player->getFileToPlay(), positionMs, positionCookie);
				}
				else {
					pr.sendNoTracksPlaying();								
				}
			}
		}
		else {

			bool reqStatus = true;
			std::stringstream reqStatusDesc;

			PlayerCommandMsg reqMsg;
			std::string msgStr(static_cast<char*>(request.data()), request.size());
			bool success = reqMsg.ParseFromString(msgStr);
			if(!success) {
				reqStatus = false;
				reqStatusDesc << "Failed to parse protobuf message";
			}
			else {

				std::cout << "Got player request message " << std::endl << reqMsg.DebugString() << std::endl;

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
						try {
							player->initialize(wavDir, newSongReq.song_name());		
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
						positionCookie++;
						std::cout << "new position cookie is: " << positionCookie << std::endl;
					}

				}
			}

			PlayerCommandReplyMsg replyProtoMsg;
			replyProtoMsg.mutable_req_identifier()->CopyFrom(reqMsg.req_identifier());
			replyProtoMsg.set_req_status(reqStatus);
			replyProtoMsg.set_req_status_desc(reqStatusDesc.str());
			replyProtoMsg.set_is_song_playing(player != NULL ? player->isPlaying() : false);
			if(player != NULL) {
				replyProtoMsg.set_current_song_path(player->getFileToPlay());
			}

			std::string replayProtoSerialized;
			replyProtoMsg.SerializeToString(&replayProtoSerialized);
			zmq::message_t reply(replayProtoSerialized.size());
			memcpy(reply.data(), replayProtoSerialized.c_str(), replayProtoSerialized.size());
			socket.send(reply);

			std::cout << "sent reply: " << std::endl << replyProtoMsg.DebugString();
		}
	}

	return 0;
}


