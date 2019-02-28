
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

#include "web_sockets_api.h"
#include "http_api.h"
#include "player_events_ifc.h"
#include "player_actions_ifc.h"
#include "alsa_frames_transfer.h"


class AlsaPlayerHandler : 
	public wavplayeralsa::PlayerActionsIfc,
	public wavplayeralsa::PlayerEventsIfc 
{

public:

	void Initialize(boost::asio::io_service *statusReporterIos, wavplayeralsa::WebSocketsApi *statusReporter, const std::string &wavDir) {
		m_wavDir = boost::filesystem::path(wavDir);
		m_statusReporterIos = statusReporterIos;
		m_statusReporter = statusReporter;

		m_player.Initialize(this, "default");
	}

public:
	bool NewSongRequest(const std::string &file_id, uint64_t start_offset_ms, std::stringstream &out_msg) {

		if(file_id == m_player.GetFileId()) {
			out_msg << "changed position of the current file '" << file_id << "'. new position in ms is: " << start_offset_ms << std::endl;
		}
		else {

			// create the canonical full path of the file to play
			boost::filesystem::path songPathInWavDir(file_id);
			boost::filesystem::path songFullPath = m_wavDir / songPathInWavDir;
			std::string canonicalFullPath;
			try {
			 	canonicalFullPath = boost::filesystem::canonical(songFullPath).string();
			}
			catch (const std::exception &e) {
				out_msg << "loading new audio file '" << file_id << "' failed. error: " << e.what();
				return false;
			}

			try {
				m_player.LoadNewFile(canonicalFullPath, file_id);
				out_msg << "song successfully changed to '" << file_id << "'. " <<
						"new audio file will start playing at position " << start_offset_ms << " ms";
			}
			catch(const std::runtime_error &e) {
				out_msg << "loading new audio file '" << file_id << "' failed. currently no audio file is loaded in the player and it is not playing. " <<
					"reason for failure: " << e.what();
				return false;
			}
		}

		m_player.StartPlay(start_offset_ms);
		return true;
	}

	bool StopPlayRequest(std::stringstream &out_msg) {
		try {
			m_player.Stop();
		}
		catch(const std::runtime_error &e) {
			out_msg << "Unable to stop current song successfully, error: " << e.what();
			return false;
		}
		out_msg << "current song '" << m_player.GetFileId() << "' stopped playing";
		return true;
	}

public:
	void NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed) {
		m_statusReporterIos->post(boost::bind(&wavplayeralsa::WebSocketsApi::NewSongStatus, m_statusReporter, file_id, start_time_millis_since_epoch, speed));
	}

	void NoSongPlayingStatus() {
		m_statusReporterIos->post(boost::bind(&wavplayeralsa::WebSocketsApi::NoSongPlayingStatus, m_statusReporter));		
	}	

private:
	wavplayeralsa::AlsaFramesTransfer m_player;
	boost::filesystem::path m_wavDir;
	
	boost::asio::io_service *m_statusReporterIos;
	wavplayeralsa::WebSocketsApi *m_statusReporter;

};


/*
This is the main manager class that creates, initialize, and start all the relevant components
*/
class WavPlayerAlsa {

public:

	WavPlayerAlsa() :
		io_service_work_(io_service_)
	{

	}

	void CommandLineArguments(int argc, char *argv[]) {
		// current directory
		char cwdCharArr[PATH_MAX];
		getcwd(cwdCharArr, sizeof(cwdCharArr));


		cxxopts::Options options("wavplayeralsa", "wav files player with accurate position in audio tracking.");
		options.add_options()
			("f,initial_file", "file which will be played on run", cxxopts::value<std::string>())
			("d,wav_dir", "the directory in which wav files are located", cxxopts::value<std::string>()->default_value(cwdCharArr))
			("ws_listen_port", "port on which player listen for websocket clients, to send internal event updates", cxxopts::value<uint16_t>()->default_value("9002"))
			("http_listen_port", "port on which player listen for http clients, to receive external commands and send state", cxxopts::value<uint16_t>()->default_value("8080"))
			("log_dir", "directory for log file (directory must exist, will not be created)", cxxopts::value<std::string>())
			("h, help", "print help")
			;

		try {

			auto cmd_line_parameters = options.parse(argc, argv);

			if(cmd_line_parameters.count("help")) {
			 	std::cout << options.help({""}) << std::endl;
			 	exit(EXIT_SUCCESS);
			}

			// parse options
			save_logs_to_file_ = (cmd_line_parameters.count("log_dir") > 0);
			if(save_logs_to_file_) {
				log_dir_ = cmd_line_parameters["log_dir"].as<std::string>();
			}
			if(cmd_line_parameters.count("initial_file") > 0) {
		 		initial_file_ = cmd_line_parameters["initial_file"].as<std::string>();
			}
			ws_listen_port_ = cmd_line_parameters["ws_listen_port"].as<uint16_t>();
			http_listen_port_ = cmd_line_parameters["http_listen_port"].as<uint16_t>();
			wav_dir_ = cmd_line_parameters["wav_dir"].as<std::string>();

		}
		catch(const cxxopts::OptionException &e) {
			std::cerr << "Invalid command line options: '" << e.what() << "'" << std::endl;
			exit(EXIT_FAILURE);
		}
		catch(const std::runtime_error &e) {
			std::cerr << "general error: '" << e.what() << "'" << std::endl;
			exit(EXIT_FAILURE);
		}

	}

	// create an initialize all the required loggers.
	// we do not want to work without loggers.
	// if the function is unable to create a logger, it will print to stderr, and terminate the application
	void CreateLoggers(const char *command_name) {

		try {

			// create two logger sinks
			std::vector<spdlog::sink_ptr> sinks;
			sinks.push_back(createLoggerConsole());
			if(save_logs_to_file_) {
				sinks.push_back(createLoggerFileSink(command_name, log_dir_));
			}

			root_logger_ = std::make_shared<spdlog::logger>("root", sinks.begin(), sinks.end());
			root_logger_->flush_on(spdlog::level::info); 

			root_logger_->info("hello wavplayeralsa. logger initialized");
			if(save_logs_to_file_) {
				root_logger_->info("log file for this run is: '{}'", m_logFilePath);
				root_logger_->info("canonical location of log file: '{}'", m_logFileCanonicalPath);
			}
			root_logger_->info("pid of player: {}", getpid());

			http_api_logger_ = root_logger_->clone("http_api");
			ws_api_logger_ = root_logger_->clone("ws_api");
		}
		catch(const std::exception &e) {
			std::cerr << "Unable to create loggers. error is: " << e.what() << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	// can throw exception
	void InitializeComponents() {
		try {
			web_sockets_api_.Initialize(ws_api_logger_, &io_service_, ws_listen_port_);
			http_api_.Initialize(http_api_logger_, &io_service_, &alsa_player_handler, http_listen_port_);
			alsa_player_handler.Initialize(&io_service_, &web_sockets_api_, wav_dir_);			
		}
		catch(const std::exception &e) {
			root_logger_->critical("failed initialization, unable to start player. {}", e.what());
			exit(EXIT_FAILURE);			
		}
	}

	void Start() {

		if(!initial_file_.empty()) {	
		 	std::stringstream initial_file_play_status;
			bool success = alsa_player_handler.NewSongRequest(initial_file_, 0, initial_file_play_status);
			if(!success) {
				root_logger_->error("unable to play initial file. {}", initial_file_play_status.str());
				exit(EXIT_FAILURE);
			}
		}	

		io_service_.run();
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

private:
	// app infra
	boost::asio::io_service io_service_;
	boost::asio::io_service::work io_service_work_;

private:
	// command line arguments
	bool save_logs_to_file_;
	std::string log_dir_;
	std::string initial_file_;
	uint16_t ws_listen_port_;
	uint16_t http_listen_port_;
	std::string wav_dir_;

private:
	// loggers
	std::shared_ptr<spdlog::logger> root_logger_;
	std::shared_ptr<spdlog::logger> http_api_logger_;
	std::shared_ptr<spdlog::logger> ws_api_logger_;


private:
	// app components
	wavplayeralsa::WebSocketsApi web_sockets_api_;
	wavplayeralsa::HttpApi http_api_;
	AlsaPlayerHandler alsa_player_handler;

};


int main(int argc, char *argv[]) {

	WavPlayerAlsa wav_player_alsa;
	wav_player_alsa.CommandLineArguments(argc, argv);
	wav_player_alsa.CreateLoggers(argv[0]);
	wav_player_alsa.InitializeComponents();
	wav_player_alsa.Start();

	return 0;
}


