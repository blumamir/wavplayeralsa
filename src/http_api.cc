
#include "http_api.h"

#include <nlohmann/json.hpp>


using json = nlohmann::json;


namespace wavplayeralsa {

	void HttpApi::Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, PlayerActionsIfc *player_action_callback, uint16_t http_listen_port) {

		// set class members
		player_action_callback_ = player_action_callback;
		logger_ = logger;

	  	server_.config.port = http_listen_port;
	  	server_.io_service = std::shared_ptr<boost::asio::io_service>(io_service);
		server_.resource["^/current-song$"]["PUT"] = std::bind(&HttpApi::OnPutCurrentSong, this, std::placeholders::_1, std::placeholders::_2);

		try {
	  		server_.start();
	  	}
	  	catch(const std::exception &e) {
	  		std::stringstream err_msg;
	    	err_msg << "http server 'start' on port " << http_listen_port << " failed, probably not able to bind to port. error msg: " << e.what();
	    	throw std::runtime_error(err_msg.str());
	  	}

	  	logger_->info("http server started on port {}", http_listen_port);
	}

	void HttpApi::WriteResponseBadRequest(std::shared_ptr<HttpServer::Response> response, const std::stringstream &err_stream)
	{
		std::string err_msg = err_stream.str();
		*response << "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: " << err_msg.size() << "\r\n\r\n" << err_msg;		
	  	logger_->info("http request failed. returning error string: {}", err_msg);
	}

	void HttpApi::OnPutCurrentSong(std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
		std::string request_json_str = request->content.string();
		logger_->info("http received put request for current-song: {}", request_json_str);

		// parse to json
		json j;
		try {
			j = json::parse(request_json_str);
		}
		catch(json::exception &e) {
			std::stringstream err_stream;
			err_stream << "http request content is not a json string. error msg: '" << e.what() << "'";
			WriteResponseBadRequest(response, err_stream);
		    return;
		}

		// validate song name
		std::string song_name;
		if(j.find("song_name") != j.end()) {
			try {
				song_name = j["song_name"].get<std::string>();
			}
			catch(json::exception &e) {
				std::stringstream err_stream;
				err_stream << "cannot find valid value for 'song_name' in request json. error msg: '" << e.what() << "'";
				WriteResponseBadRequest(response, err_stream);
			    return;
			}
		}

		uint64_t start_offset_ms = 0;
		// use it only if it is found in the json
		if(j.find("start_offset_ms") != j.end()) {
			try {
				start_offset_ms = j["start_offset_ms"].get<uint64_t>();
			}
			catch(json::exception &e) {
				std::stringstream err_stream;
				err_stream << "cannot find valid value for 'start_offset_ms' in request json. error msg: '" << e.what() << "'";
				WriteResponseBadRequest(response, err_stream);
			    return;
			}
		}

		std::stringstream handler_msg;
		bool success;
		if(song_name.empty()) {
			success = player_action_callback_->StopPlayRequest(handler_msg);
		}
		else {
			success = player_action_callback_->NewSongRequest(song_name, start_offset_ms, handler_msg);	
		} 

		if(!success) {
			WriteResponseBadRequest(response, handler_msg);
		    return;			
		}

	    response->write(handler_msg);
	    logger_->info("http put request for current-song succeeded: {}", handler_msg.str());		
	}

}