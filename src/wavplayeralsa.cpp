
#include "single_file_player.h"
#include <iostream>
#include <sndfile.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <linux/limits.h>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include "cxxopts/cxxopts.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

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

	void NoSongPlayingStatus() {
		m_statusReporterIos->post(boost::bind(&wavplayeralsa::StatusReporterIfc::NoSongPlayingStatus, m_statusReporter));		
	}

private:
	boost::asio::io_service *m_statusReporterIos;
	wavplayeralsa::StatusReporterIfc *m_statusReporter;

};

class AlsaPlayerHandler : public wavplayeralsa::PlayerRequestIfc {

public:

	AlsaPlayerHandler(ThreadsRouter *threadRounter) :
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


/*
This is the main manager class that creates, initialize, and start all the relevant components
*/
class WavPlayerAlsa {

public:

	// create an initialize all the required loggers.
	// we do not want to work without loggers.
	// if the function is unable to create a logger, it will print to stderr, and terminate the application
	void createLoggers(const char *commandName, bool saveLogsToFile, const std::string &logDir) {

		try {

			// create two logger sinks
			std::vector<spdlog::sink_ptr> sinks;
			sinks.push_back(createLoggerConsole());
			if(saveLogsToFile) {
				sinks.push_back(createLoggerFileSink(commandName, logDir));
			}

			spdlog::logger logger("root", sinks.begin(), sinks.end());
			logger.info("hello wavplayeralsa. logger initialized");
			if(saveLogsToFile) {
				logger.info("log file for this run is: '{}'", m_logFilePath);
				logger.info("canonical location of log file: '{}'", m_logFileCanonicalPath);
			}
			logger.info("pid of player: {}", getpid());

			// std::shared_ptr<spdlog::logger> alsaLogger = logger.clone("alsa");
			// alsaLogger->info("loggers initialized");			
		}
		catch(const std::exception &e) {
			std::cerr << "Unable to create loggers. error is: " << e.what() << std::endl;
			exit(EXIT_FAILURE);
		}
	}

private:

	// create a file sink for logging.
	// each invokation of the program will create a new file, with a name that contains the
	// current date and time.
	//
	// commandName is argv[0], something like './bin/wavplayeralsa'. log will use the program name
	// from this string (e.g. will drop the directories)
	// 
	// logDir is a directory path for where we want to store log files.
	// it must exist, the function will not create it (actually, this is the behaviour of the
	// logging library in use)
	spdlog::sink_ptr createLoggerFileSink(const char *commandName, const std::string &logDir) {

		// get executable name from command used to invoke program
		boost::filesystem::path boostCommandName(commandName);
		std::string exeName = boostCommandName.stem().string();

		// get current time
	    auto t = std::time(nullptr);
	    auto tm = *std::localtime(&t);

	    //make file name
	    std::stringstream logFileNameStream;
	    logFileNameStream << exeName << "_" << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S") << ".txt";

	    // combine with directory
	    boost::filesystem::path boostDir(logDir);
	    boost::filesystem::path boostFile(logFileNameStream.str());
	    boost::filesystem::path logFilePath = boostDir / boostFile;
	    m_logFilePath = logFilePath.string();
		spdlog::sink_ptr loggerFileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(m_logFilePath);
		m_logFileCanonicalPath = boost::filesystem::canonical(logFilePath).string();
		return loggerFileSink;
	}

	spdlog::sink_ptr createLoggerConsole() {
		return std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	}

private:
	std::string m_logFilePath; // might be ralative path, or contain ugly things like /// . .. etc
	std::string m_logFileCanonicalPath; // an absolute path that has no dot, dot-dot elements or symbolic links in its generic format representation


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
		("http_interface_port", "port on which player listen for http interface (requests and status)", cxxopts::value<uint16_t>()->default_value("80"))
		("log_dir", "directory for log file (directory must exist, will not be created)", cxxopts::value<std::string>())
		("h, help", "print help")
		;

	boost::asio::io_service io_service;
	boost::asio::io_service::work work(io_service);

	ThreadsRouter threadsRouter;
	threadsRouter.initialize(&io_service, statusReporter);

	AlsaPlayerHandler alsaPlayerHandler(&threadsRouter);

	uint16_t statusReporterPort = 9002;
	uint16_t httpInterfacePort = 80;
	std::string initialFile;
	bool saveLogsToFile = false;
	std::string logDir;

	try {

		 auto optsresult = options.parse(argc, argv);

		 if(optsresult.count("help")) {
		 	std::cout << options.help({""}) << std::endl;
		 	return 0;
		 }

		 // status reporter stuff
		 statusReporterPort = optsresult["status_report_port"].as<uint16_t>();
		 httpInterfacePort = optsresult["http_interface_port"].as<uint16_t>();

		 // player stuff
		 alsaPlayerHandler.setWavDir(optsresult["wav_dir"].as<std::string>());
		 if(optsresult.count("initial_file")) {
		 	initialFile = optsresult["initial_file"].as<std::string>();
		 }

		 //logger
		 if(optsresult.count("log_dir")) {
		 	logDir = optsresult["log_dir"].as<std::string>();
		 	saveLogsToFile = true;
		 }

	}
	catch(const cxxopts::OptionException &e) {
		std::cout << "Invalid command line options: '" << e.what() << "'" << std::endl;
		return -1;
	}
	catch(const std::runtime_error &e) {
		std::cout << "general error: '" << e.what() << "'" << std::endl;
		return -1;
	}

	WavPlayerAlsa wavPlayerAlsa;
	wavPlayerAlsa.createLoggers(argv[0], saveLogsToFile, logDir);


	statusReporter->Configure(&io_service, statusReporterPort);
	playerReqHttp.Initialize(httpInterfacePort, &io_service, &alsaPlayerHandler);


	if(!initialFile.empty()) {
		std::stringstream initialFilePlayStatus;
		bool success = alsaPlayerHandler.NewSongRequest(initialFile, 0, initialFilePlayStatus);
	}

	io_service.run();
	
	return 0;
}


