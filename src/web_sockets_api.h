#ifndef __STATUS_REPORTER_IFC_H__
#define __STATUS_REPORTER_IFC_H__

#include <cstdint>
#include <set>

#include <boost/asio.hpp>
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"
#include "spdlog/spdlog.h"

#include "player_events_ifc.h"

namespace wavplayeralsa {

	class WebSocketsApi {

	public:
		WebSocketsApi();

	public:
		void Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, uint16_t ws_listen_port);

	public:
		void ReportCurrentSong(const std::string &json_str);

	private:
		// web sockets callbacks
		void OnOpen(websocketpp::connection_hdl hdl);
		void OnClose(websocketpp::connection_hdl hdl);

	private:

		websocketpp::server<websocketpp::config::asio> server_;
		std::shared_ptr<spdlog::logger> logger_;
		boost::asio::io_service *io_service_;

		typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> ConList;
		ConList connections_;

		bool initialized = false;

		std::string last_status_msg_;
	};

}

#endif // __STATUS_REPORTER_IFC_H__