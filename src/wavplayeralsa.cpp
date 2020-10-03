
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

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
#include "services/config_service.h"


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

		bool successfull = config_service_.InitFromCmdArguments(argc, argv);
		if(!successfull) {
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
			if(config_service_.SaveLogsToFile()) {
				sinks.push_back(createLoggerFileSink(command_name, config_service_.GetLogDir()));
			}

			root_logger_ = std::make_shared<spdlog::logger>("root", sinks.begin(), sinks.end());
			root_logger_->flush_on(spdlog::level::info); 

			root_logger_->info("hello wavplayeralsa. logger initialized");
			if(config_service_.SaveLogsToFile()) {
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

	void LogConfig() {
		config_service_.LogConfig(root_logger_);
	}

	void CreateUUID() {
		uuid_ = boost::uuids::to_string(boost::uuids::random_generator()());
		root_logger_->info("Generated uuid for this player run: {}", uuid_);
	}

	// can throw exception
	void InitializeComponents() {
		try {
			audio_files_manager.Initialize(config_service_.GetWavDir());
			web_sockets_api_.Initialize(ws_api_logger_, &io_service_, config_service_.GetWsListenPort());
			http_api_.Initialize(http_api_logger_, uuid_, &io_service_, &current_song_controller_, &audio_files_manager, config_service_.GetHttpListenPort());

			// controllers
			current_song_controller_.Initialize(uuid_, config_service_.GetWavDir());

			// services

			alsa_playback_service_factory_.Initialize(
				alsa_playback_service_factory_logger,
				&current_song_controller_,
				config_service_.GetAudioDevice()
			);

			if(config_service_.UseMqtt()) {
				mqtt_api_.Initialize(mqtt_api_logger_, config_service_.GetMqttHost(), config_service_.GetMqttPort());
			}
		}
		catch(const std::exception &e) {
			root_logger_->critical("failed initialization, unable to start player. {}", e.what());
			exit(EXIT_FAILURE);			
		}
	}

	void Start() {

		if(!config_service_.GetInitialFile().empty()) {	
		 	std::stringstream initial_file_play_status;
			bool success = current_song_controller_.NewSongRequest(config_service_.GetInitialFile(), 0, initial_file_play_status, nullptr);
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
	wavplayeralsa::ConfigService config_service_;

	wavplayeralsa::CurrentSongController current_song_controller_;

};


int main(int argc, char *argv[]) {

	WavPlayerAlsa wav_player_alsa;
	wav_player_alsa.CommandLineArguments(argc, argv);
	wav_player_alsa.CreateLoggers(argv[0]);
	wav_player_alsa.LogConfig();
	wav_player_alsa.CreateUUID();
	wav_player_alsa.InitializeComponents();
	wav_player_alsa.Start();

	return 0;
}


