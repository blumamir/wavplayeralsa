
#include "single_file_player.h"
#include "generated/player_command.pb.h"
#include <iostream>
#include <sndfile.h>
#include <cxxopts.hpp>

#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <linux/limits.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <status_reporter_ifc.hpp>
#include <player_req_http.hpp>
#include "status_update_msg.h"


class ThreadsRouter : public wavplayeralsa::StatusUpdateMsg {

public:

	void initialize(boost::asio::io_service *statusReporterIos, wavplayeralsa::StatusReporterIfc *statusReporter) {
		m_statusReporterIos = statusReporterIos;
		m_statusReporter = statusReporter;
	}

public:
	void NewSongStatus(const std::string &songName, uint64_t startTimeMs, double speed) {
		m_statusReporterIos->post(boost::bind(&wavplayeralsa::StatusReporterIfc::NewSongStatus, m_statusReporter, songName, startTimeMs, speed));
	}

private:
	boost::asio::io_service *m_statusReporterIos;
	wavplayeralsa::StatusReporterIfc *m_statusReporter;

};

class NewSongHandler : public wavplayeralsa::PlayerRequestIfc {

public:

	NewSongHandler(ThreadsRouter *threadRounter) :
		m_threadsRouter(threadRounter)
	{

	}

	void setWavDir(const std::string &wavDir) {
		m_wavDir = wavDir;
	}

public:
	bool NewSongRequest(const std::string &songName, uint64_t startOffsetMs, std::stringstream &outMsg) {

		if(m_player != NULL && songName == m_player->getFileToPlay()) {
			outMsg << "changed position of the current song '" << songName << "'. new position in ms is: " << startOffsetMs << std::endl;
		}
		else {
			// so the request asks to change the song.
			// we will first delete the previous song, then try to load the new one.
			// in case we fail, we will be left with no song at all which is fine

			if(m_player != NULL) {
				m_player->stop();
				delete m_player;
			}

			m_player = new wavplayeralsa::SingleFilePlayer();
			try {
				m_player->initialize(m_wavDir, songName, m_threadsRouter);		
				outMsg << "song successfully changed to '" << songName << "'. " <<
						"new song will start playing at position " << startOffsetMs << " ms";
			}
			catch(const std::runtime_error &e) {
				delete m_player;
				m_player = NULL;
				outMsg << "loading new song '" << songName << "' failed. currently no song is loaded in the player and it is not playing. " <<
					"reason for failure: " << e.what();
				return false;
			}
		}

		if(m_player != NULL) {
			m_player->startPlay(startOffsetMs);
		}

		return true;
	}

	bool StopPlayRequest(std::stringstream &outMsg) {
		if(m_player != NULL) {
			m_player->stop();
			outMsg << "current song '" << m_player->getFileToPlay() << "' stopped playing";
		}
		else {
			outMsg << "asked to stop song, and there is no song currently playing";
		}
		return true;
	}

private:
	wavplayeralsa::SingleFilePlayer *m_player = NULL;
	ThreadsRouter *m_threadsRouter;
	std::string m_wavDir;

};


int main(int argc, char *argv[]) {

	// current directory
	char cwdCharArr[PATH_MAX];
	getcwd(cwdCharArr, sizeof(cwdCharArr));

	wavplayeralsa::StatusReporterIfc *statusReporter = wavplayeralsa::CreateStatusReporter();
	wavplayeralsa::PlayerReqHttp playerReqHttp;


	cxxopts::Options options("wavplayeralsa", "wav files player with accurate position in audio tracking.");
	options.add_options()
		("f,initial_file", "file which will be played on run", cxxopts::value<std::string>())
		("d,wav_dir", "the directory in which wav files are located", cxxopts::value<std::string>()->default_value(cwdCharArr))
		("status_report_port", "port on which player opens websocket for status updates to clients", cxxopts::value<uint16_t>()->default_value("9002"))
		("position_report_port", "port on which live position messages are published over broadcast", cxxopts::value<uint16_t>()->default_value("2001"))
		("position_report_rate", "the rate for position report messages in ms", cxxopts::value<uint32_t>()->default_value("5"))
		("cmd_ifc_port", "port to listen for player commands over network", cxxopts::value<uint16_t>()->default_value("2100"))
		("h, help", "print help")
		;

	boost::asio::io_service io_service;
	boost::asio::io_service::work work(io_service);

	ThreadsRouter threadsRouter;
	threadsRouter.initialize(&io_service, statusReporter);

	NewSongHandler newSongHandler(&threadsRouter);

	int rcvtimeo = 5;
	uint16_t cmdIfcPort = 2100;
	uint16_t statusReporterPort = 9002;
	std::string initialFile;

	try {

		 auto optsresult = options.parse(argc, argv);

		 if(optsresult.count("help")) {
		 	std::cout << options.help({""}) << std::endl;
		 	return 0;
		 }

		 // status reporter stuff
		 statusReporterPort = optsresult["status_report_port"].as<uint16_t>();

		 // player stuff
		 newSongHandler.setWavDir(optsresult["wav_dir"].as<std::string>());
		 if(optsresult.count("initial_file")) {
		 	initialFile = optsresult["initial_file"].as<std::string>();
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


	statusReporter->Configure(&io_service, statusReporterPort);
	playerReqHttp.Initialize(8080, &io_service, &newSongHandler);


	if(!initialFile.empty()) {
		std::stringstream initialFilePlayStatus;
		bool success = newSongHandler.NewSongRequest(initialFile, 0, initialFilePlayStatus);
	}

	io_service.run();
	
	return 0;
}


