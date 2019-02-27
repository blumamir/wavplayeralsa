#ifndef __STATUS_REPORTER_IFC_H__
#define __STATUS_REPORTER_IFC_H__

#include <cstdint>
#include <set>

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "player_events_ifc.h"


namespace wavplayeralsa {

	class WebSocketsApi : public PlayerEventsIfc {

	public:
		void Initialize(boost::asio::io_service *ioSerivce, uint16_t wsListenPort);

	public:
		// PlayerEventsIfc
		void NewSongStatus(const std::string &songName, uint64_t startTimeMs, double speed);
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

		websocketpp::server<websocketpp::config::asio> m_server;

		typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> ConList;
		ConList m_connections;

		std::string m_lastStatusMsg;
	};

}

#endif // __STATUS_REPORTER_IFC_H__