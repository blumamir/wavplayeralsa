
#include "player_req_http.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;


namespace wavplayeralsa {

	PlayerReqHttp::PlayerReqHttp() {

	}

	void PlayerReqHttp::Initialize(uint16_t httpListenPort, boost::asio::io_service *io_service, PlayerRequestIfc *playerReqCallback) {

		// set class members
		m_httpListenPort = httpListenPort;
		m_io_service = io_service;
		m_playerReqCallback = playerReqCallback;

		// do intializations
		InitializeHttpServer();
	}

	void PlayerReqHttp::OnPutCurrentSong(std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
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

	void PlayerReqHttp::InitializeHttpServer() {
	  	m_server.config.port = m_httpListenPort;
	  	m_server.io_service = std::shared_ptr<boost::asio::io_service>(m_io_service);
		m_server.resource["^/current-song$"]["PUT"] = std::bind(&PlayerReqHttp::OnPutCurrentSong, this, std::placeholders::_1, std::placeholders::_2);
	  	m_server.start();
	}

}