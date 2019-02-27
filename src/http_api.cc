
#include "http_api.h"

#include <nlohmann/json.hpp>


using json = nlohmann::json;


namespace wavplayeralsa {

	HttpApi::HttpApi() {

	}

	void HttpApi::Initialize(boost::asio::io_service *io_service, PlayerActionsIfc *playerReqCallback, uint16_t httpListenPort) {

		// set class members
		m_httpListenPort = httpListenPort;
		m_io_service = io_service;
		m_playerReqCallback = playerReqCallback;

		// do intializations
		InitializeHttpServer();
	}

	void HttpApi::OnPutCurrentSong(std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
		std::string requestJsonStr = request->content.string();
		std::cout << "received new player request: " << requestJsonStr << std::endl;

		// parse to json
		json j;
		try {
			j = json::parse(requestJsonStr);
		}
		catch(json::exception &e) {
			std::stringstream errStream;
			errStream << "http request content is not a json string. error msg: '" << e.what() << "'";
			std::string errMsg = errStream.str();
			*response << "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: " << errMsg.size() << "\r\n\r\n" << errMsg;
		    return;
		}

		// validate song name
		std::string songName;
		if(j.find("song_name") != j.end()) {
			try {
				songName = j["song_name"].get<std::string>();
			}
			catch(json::exception &e) {
				std::stringstream errStream;
				errStream << "cannot find valid value for 'song_name' in request json. error msg: '" << e.what() << "'";
				std::string errMsg = errStream.str();
				*response << "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: " << errMsg.size() << "\r\n\r\n" << errMsg;
			    return;
			}
		}

		uint64_t startOffsetMs = 0;
		// use it only if it is found in the json
		if(j.find("start_offset_ms") != j.end()) {
			try {
				startOffsetMs = j["start_offset_ms"].get<uint64_t>();
			}
			catch(json::exception &e) {
				std::stringstream errStream;
				errStream << "cannot find valid value for 'start_offset_ms' in request json. error msg: '" << e.what() << "'";
				std::string errMsg = errStream.str();
				*response << "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: " << errMsg.size() << "\r\n\r\n" << errMsg;
			    return;
			}
		}

		std::stringstream handlerMsg;
		bool success;
		if(songName.empty()) {
			success = m_playerReqCallback->StopPlayRequest(handlerMsg);
		}
		else {
			success = m_playerReqCallback->NewSongRequest(songName, startOffsetMs, handlerMsg);	
		} 

		if(!success) {
			std::string errMsg = handlerMsg.str();
			*response << "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: " << errMsg.size() << "\r\n\r\n" << errMsg;
		    return;			
		}

	    response->write(handlerMsg);		
	}

	void HttpApi::InitializeHttpServer() {
	  	m_server.config.port = m_httpListenPort;
	  	m_server.io_service = std::shared_ptr<boost::asio::io_service>(m_io_service);
		m_server.resource["^/current-song$"]["PUT"] = std::bind(&HttpApi::OnPutCurrentSong, this, std::placeholders::_1, std::placeholders::_2);

		try {
	  		m_server.start();
	  	}
	  	catch(const std::exception &e) {
	  		std::stringstream err_msg;
	    	err_msg << "http server 'start' on port " << m_httpListenPort << " failed, probably not able to bind to port. error msg: " << e.what();
	    	throw std::runtime_error(err_msg.str());
	  	}
	}

}