
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <uuid/uuid.h>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "cxxopts/cxxopts.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "web_sockets_api.h"
#include "http_api.h"
#include "mqtt_api.h"
#include "audio_files_manager.h"
#include "current_song_controller.h"
#include "services/alsa_service.h"


/*
This is the main manager class that creates, initialize, and start all the relevant components
*/
class WavPlayerAlsa {

public:

	WavPlayerAlsa() :
		web_sockets_api_(),
		io_service_work_(io_service_),
		mqtt_api_(io_service_),
		current_song_controller_(
			io_service_, 
			&mqtt_api_, 
			&web_sockets_api_, 
			&alsa_playback_service_factory_)
	{

	}

	void CommandLineArguments(int argc, char *argv[]) {

		const std::string current_working_directory = boost::filesystem::current_path().string();

		cxxopts::Options options("wavplayeralsa", "wav files player with accurate position in audio tracking.");
		options.add_options()
			("f,initial_file", "file which will be played on run", cxxopts::value<std::string>())
			("d,wav_dir", "the directory in which wav files are located", cxxopts::value<std::string>()->default_value(current_working_directory))
			("ws_listen_port", "port on which player listen for websocket clients, to send internal event updates", cxxopts::value<uint16_t>()->default_value("9002"))
			("http_listen_port", "port on which player listen for http clients, to receive external commands and send state", cxxopts::value<uint16_t>()->default_value("8080"))
			("mqtt_host", "host for the mqtt message broker", cxxopts::value<std::string>())
			("mqtt_port", "port on which mqtt message broker listen for client connections", cxxopts::value<uint16_t>()->default_value("1883"))
			("log_dir", "directory for log file (directory must exist, will not be created)", cxxopts::value<std::string>())
			("audio_device", "audio device for playback. can be string like 'plughw:0,0'. use 'aplay -l' to list available devices", cxxopts::value<std::string>()->default_value("default"))
			("h, help", "print help")
			;

		try {

			auto cmd_line_parameters = options.parse(argc, argv);

			if(cmd_line_parameters.count("help")) {
			 	std::cout << options.help({""}) << std::endl;
			 	exit(EXIT_SUCCESS);
			}

			// parse options
			// see https://github.com/jarro2783/cxxopts/issues/146 for explnation why it has to be done like this (accessed in the try block)
			// if the issue is fixed in the future, code can be refactored so that access to arguments is done where they are needed
			save_logs_to_file_ = (cmd_line_parameters.count("log_dir") > 0);
			if(save_logs_to_file_) {
				log_dir_ = cmd_line_parameters["log_dir"].as<std::string>();
			}
			if(cmd_line_parameters.count("initial_file") > 0) {
		 		initial_file_ = cmd_line_parameters["initial_file"].as<std::string>();
			}
			ws_listen_port_ = cmd_line_parameters["ws_listen_port"].as<uint16_t>();
			http_listen_port_ = cmd_line_parameters["http_listen_port"].as<uint16_t>();
			if(cmd_line_parameters.count("mqtt_host") > 0) {
				mqtt_host_ = cmd_line_parameters["mqtt_host"].as<std::string>();
				mqtt_port_ = cmd_line_parameters["mqtt_port"].as<uint16_t>();
			}
			wav_dir_ = cmd_line_parameters["wav_dir"].as<std::string>();
			audio_device_ = cmd_line_parameters["audio_device"].as<std::string>();

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
	// we do not want to run without loggers.
	// if the function is unable to create a logger, it will print to stderr, and terminate the application
	void CreateLoggers(const char *command_name) {

		try {

			// default thread pool settings can be modified *before* creating the async logger:
			// spdlog::init_thread_pool(8192, 1); // queue with 8k items and 1 backing thread.			
			spdlog::init_thread_pool(8192, 1);

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
			mqtt_api_logger_ = root_logger_->clone("mqtt_api");
			alsa_playback_service_factory_logger = root_logger_->clone("alsa_playback_service_factory");
		}
		catch(const std::exception &e) {
			std::cerr << "Unable to create loggers. error is: " << e.what() << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	void CreateUUID() {
		uuid_t uuid;
        uuid_generate_time_safe(uuid);
		char uuid_str[37];      // ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0"
		uuid_unparse_lower(uuid, uuid_str);
		uuid_ = std::string(uuid_str);
		root_logger_->info("Generated uuid for this player run: {}", uuid_);
	}

	// can throw exception
	void InitializeComponents() {
		try {
			audio_files_manager.Initialize(wav_dir_);
			web_sockets_api_.Initialize(ws_api_logger_, &io_service_, ws_listen_port_);
			http_api_.Initialize(http_api_logger_, uuid_, &io_service_, &current_song_controller_, &audio_files_manager, http_listen_port_);

			// controllers
			current_song_controller_.Initialize(uuid_, wav_dir_);

			// services

			alsa_playback_service_factory_.Initialize(
				alsa_playback_service_factory_logger,
				&current_song_controller_,
				audio_device_
			);

			if(UseMqtt()) {
				mqtt_api_.Initialize(mqtt_api_logger_, mqtt_host_, mqtt_port_);
			}
		}
		catch(const std::exception &e) {
			root_logger_->critical("failed initialization, unable to start player. {}", e.what());
			exit(EXIT_FAILURE);			
		}
	}

	void Start() {

		if(!initial_file_.empty()) {	
		 	std::stringstream initial_file_play_status;
			bool success = current_song_controller_.NewSongRequest(initial_file_, 0, initial_file_play_status, nullptr);
			if(!success) {
				root_logger_->error("unable to play initial file. {}", initial_file_play_status.str());
				exit(EXIT_FAILURE);
			}
		}	

		io_service_.run();
	}

private:

	bool UseMqtt() {
		return !mqtt_host_.empty();
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
	std::string mqtt_host_;
	uint16_t mqtt_port_;
	std::string wav_dir_;
	std::string audio_device_;

private:
	// loggers
	std::shared_ptr<spdlog::logger> root_logger_;
	std::shared_ptr<spdlog::logger> http_api_logger_;
	std::shared_ptr<spdlog::logger> mqtt_api_logger_;
	std::shared_ptr<spdlog::logger> ws_api_logger_;
	std::shared_ptr<spdlog::logger> alsa_playback_service_factory_logger;

private:
	std::string uuid_;

private:
	// app components
	wavplayeralsa::WebSocketsApi web_sockets_api_;
	wavplayeralsa::HttpApi http_api_;
	wavplayeralsa::MqttApi mqtt_api_;
	wavplayeralsa::AudioFilesManager audio_files_manager;
	wavplayeralsa::AlsaPlaybackServiceFactory alsa_playback_service_factory_;

	wavplayeralsa::CurrentSongController current_song_controller_;

};


int main(int argc, char *argv[]) {

	WavPlayerAlsa wav_player_alsa;
	wav_player_alsa.CommandLineArguments(argc, argv);
	wav_player_alsa.CreateLoggers(argv[0]);
	wav_player_alsa.CreateUUID();
	wav_player_alsa.InitializeComponents();
	wav_player_alsa.Start();

	return 0;
}


