#include "services/config_service.h"

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include "cxxopts/cxxopts.hpp"

namespace wavplayeralsa
{

ConfigService::ConfigService()
{
	const std::string current_working_directory = boost::filesystem::current_path().string();
	wav_dir_ = current_working_directory; // default value
}

bool ConfigService::InitFromCmdArguments(int argc, char *argv[])
{

	cxxopts::Options options("wavplayeralsa", "wav files player with accurate position in audio tracking.");
	options.add_options()
		("c,config_file", "config file", cxxopts::value<std::string>())
		("f,initial_file", "file which will be played on run", cxxopts::value<std::string>())
		("d,wav_dir", "the directory in which wav files are located", cxxopts::value<std::string>()->default_value(wav_dir_))
		("ws_listen_port", "port on which player listen for websocket clients, to send internal event updates", cxxopts::value<uint16_t>()->default_value(std::to_string(ws_listen_port_)))
		("http_listen_port", "port on which player listen for http clients, to receive external commands and send state", cxxopts::value<uint16_t>()->default_value(std::to_string(http_listen_port_)))
		("mqtt_host", "host for the mqtt message broker", cxxopts::value<std::string>())
		("mqtt_port", "port on which mqtt message broker listen for client connections", cxxopts::value<uint16_t>()->default_value(std::to_string(mqtt_port_)))
		("log_dir", "directory for log file (directory must exist, will not be created)", cxxopts::value<std::string>())
		("audio_device", "audio device for playback. can be string like 'plughw:0,0'. use 'aplay -l' to list available devices", cxxopts::value<std::string>()->default_value(audio_device_))
		("h, help", "print help");

	try
	{

		auto cmd_line_parameters = options.parse(argc, argv);

		if (cmd_line_parameters.count("help"))
		{
			std::cout << options.help({""}) << std::endl;
			exit(EXIT_SUCCESS);
		}

		// parse options
		// see https://github.com/jarro2783/cxxopts/issues/146 for explnation why it has to be done like this (accessed in the try block)
		// if the issue is fixed in the future, code can be refactored so that access to arguments is done where they are needed

		if (cmd_line_parameters.count("config_file") > 0)
		{
			config_file_ = cmd_line_parameters["config_file"].as<std::string>();
			LoadConfigFile(config_file_);
		}

		if (cmd_line_parameters.count("log_dir") > 0)
		{
			log_dir_ = cmd_line_parameters["log_dir"].as<std::string>();
		}
		if (cmd_line_parameters.count("initial_file") > 0)
		{
			initial_file_ = cmd_line_parameters["initial_file"].as<std::string>();
		}
		if (cmd_line_parameters.count("ws_listen_port") > 0)
		{
			ws_listen_port_ = cmd_line_parameters["ws_listen_port"].as<uint16_t>();
		}
		if (cmd_line_parameters.count("http_listen_port"))
		{
			http_listen_port_ = cmd_line_parameters["http_listen_port"].as<uint16_t>();
		}
		if (cmd_line_parameters.count("mqtt_host") > 0)
		{
			mqtt_host_ = cmd_line_parameters["mqtt_host"].as<std::string>();
			mqtt_port_ = cmd_line_parameters["mqtt_port"].as<uint16_t>();
		}
		if (cmd_line_parameters.count("wav_dir") > 0)
		{
			wav_dir_ = cmd_line_parameters["wav_dir"].as<std::string>();
		}
		if (cmd_line_parameters.count("audio_device") > 0)
		{
			audio_device_ = cmd_line_parameters["audio_device"].as<std::string>();
		}
	}
	catch (const cxxopts::OptionException &e)
	{
		std::cerr << "Invalid command line options: '" << e.what() << "'" << std::endl;
		return false;
	}
	catch (const std::runtime_error &e)
	{
		std::cerr << "general error: '" << e.what() << "'" << std::endl;
		return false;
	}

	return true;
}

void ConfigService::LogConfig(std::shared_ptr<spdlog::logger> logger) const
{
	std::stringstream config_stream;
	config_stream << "Configuration:" << std::endl;
	if(HasConfigFile()) {
		config_stream << "config file: '" << config_file_ << "'" << std::endl;
	}

	config_stream << "player: wav_dir='" << wav_dir_ << "'";
	if(!initial_file_.empty()) {
		config_stream << " initial file to play: '" << initial_file_ << "'";
	}
	config_stream << std::endl;

	config_stream << "web sockets: listen_port='" << ws_listen_port_ << "'" << std::endl;

	config_stream << "http: listen_port='" << http_listen_port_ << "'" << std::endl;

	if(UseMqtt()) {
		config_stream << "mqtt: host='" << mqtt_host_ << "', port='" << mqtt_port_ << "'" << std::endl;
	}
	else {
		config_stream << "mqtt: disabled" << std::endl;
	}

	if(SaveLogsToFile()) {
		config_stream << "log file: directory='" << log_dir_ << "'" << std::endl;
	}
	else {
		config_stream << "log file: not saving log to file, as none is configured" << std::endl;
	}

	config_stream << "audio device: '" << audio_device_ << "'";
	logger->info(config_stream.str());
}

void ConfigService::SetParamFromFile(const std::string &param_name, const std::string &param_value)
{
	if (param_name == "initial_file")
	{
		initial_file_ = param_value;
	}
	else if (param_name == "wav_dir")
	{
		wav_dir_ = param_value;
	}
	else if (param_name == "ws_listen_port")
	{
		ws_listen_port_ = boost::lexical_cast<uint16_t>(param_value);
	}
	else if (param_name == "http_listen_port")
	{
		http_listen_port_ = boost::lexical_cast<uint16_t>(param_value);
	}
	else if (param_name == "mqtt_host")
	{
		mqtt_host_ = param_value;
	}
	else if (param_name == "mqtt_port")
	{
		mqtt_port_ = boost::lexical_cast<uint16_t>(param_value);
	}
	else if (param_name == "log_dir")
	{
		log_dir_ = param_value;
	}
	else if (param_name == "audio_device")
	{
		audio_device_ = param_value;
	}
	else
	{
		std::stringstream err;
		err << "got unexpected option in config file: '" << param_name << "'";
		throw std::runtime_error(err.str());
	}
}

void ConfigService::LoadConfigFile(const std::string &path)
{
	std::ifstream infile(path);
	std::string line;
	while (std::getline(infile, line))
	{

		std::istringstream line_stream(line);

		std::string param_name, param_value;
		line_stream >> param_name >> param_value;
		if (param_name.empty() || param_name[0] == ';' || param_name[0] == '#')
			continue;

		if (param_value.empty())
		{
			infile.close();
			std::stringstream err;
			err << "config file: no value for option '" << param_name << "'";
			throw std::runtime_error(err.str());
		}

		try
		{
			SetParamFromFile(param_name, param_value);
		}
		catch (const boost::bad_lexical_cast &e)
		{
			infile.close();
			std::stringstream errStream;
			errStream << "config file: invalid value '" << param_value << "' for parameter '" << param_name << "'";
			throw std::runtime_error(errStream.str());
		}
	}

	infile.close();
}

} // namespace wavplayeralsa