#ifndef WAVPLAYERALSA_HTTP_API_H_
#define WAVPLAYERALSA_HTTP_API_H_

#include <cstdint>

#include <boost/asio.hpp>
#include "simple-web-server/server_http.hpp"

#include "player_actions_ifc.h"


using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace wavplayeralsa {

	/*
	Interface to receive player requests via http server
	*/
	class HttpApi {

	public:
		HttpApi();

		void Initialize(boost::asio::io_service *io_service, PlayerActionsIfc *playerReqCallback, uint16_t httpListenPort);

	private:
		void InitializeHttpServer();

	private:
		void OnPutCurrentSong(std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request);


	private:
		// outside configurartion
		uint16_t m_httpListenPort;
		boost::asio::io_service *m_io_service;		
		PlayerActionsIfc *m_playerReqCallback;

	private:
		// class private members
		HttpServer m_server;

	};

	
}

#endif // WAVPLAYERALSA_HTTP_API_H_