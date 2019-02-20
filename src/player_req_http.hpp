#include "player_req_ifc.hpp"

#include <boost/asio.hpp>
#include <cstdint>

#include "simple-web-server/server_http.hpp"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace wavplayeralsa {

	/*
	Interface to receive player requests via http server
	*/
	class PlayerReqHttp {

	public:
		PlayerReqHttp();

		void Initialize(uint16_t httpListenPort, boost::asio::io_service *io_service, PlayerRequestIfc *playerReqCallback);

	private:
		void InitializeHttpServer();

	private:
		void OnPutCurrentSong(std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request);


	private:
		// outside configurartion
		uint16_t m_httpListenPort;
		boost::asio::io_service *m_io_service;		
		PlayerRequestIfc *m_playerReqCallback;

	private:
		// class private members
		HttpServer m_server;

	};

	
}