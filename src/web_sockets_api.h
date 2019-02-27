#ifndef __STATUS_REPORTER_IFC_H__
#define __STATUS_REPORTER_IFC_H__

#include <cstdint>
#include <set>

#include <boost/asio.hpp>
#include "nlohmann/json.hpp"
#include "websocketpp/config/asio_no_tls.hpp"
#include "websocketpp/server.hpp"
#include "spdlog/spdlog.h"

#include "player_events_ifc.h"


namespace wavplayeralsa {

	class WebSocketsApi : public PlayerEventsIfc {

	public:
		void Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, uint16_t ws_listen_port);

	public:
		// PlayerEventsIfc
		void NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed);
		void NoSongPlayingStatus();

	private:
		// handler functions
		void UpdateLastStatusMsg(const nlohmann::json &msgJson);
		void SendToAllConnectedClients();

	private:
		// web sockets callbacks
		void OnOpen(websocketpp::connection_hdl hdl);
		void OnClose(websocketpp::connection_hdl hdl);

	private:

		websocketpp::server<websocketpp::config::asio> server_;
		std::shared_ptr<spdlog::logger> logger_;

		typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> ConList;
		ConList connections_;

		std::string last_status_msg_;
	};

}

#endif // __STATUS_REPORTER_IFC_H__