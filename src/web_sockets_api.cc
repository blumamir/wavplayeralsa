#include "web_sockets_api.h"

#include <boost/foreach.hpp>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::connection_hdl;

using json = nlohmann::json;


namespace wavplayeralsa {


	void WebSocketsApi::Configure(boost::asio::io_service *ioSerivce, uint16_t wsListenPort) {

	    m_server.clear_error_channels(websocketpp::log::alevel::all);
	    m_server.clear_access_channels(websocketpp::log::alevel::all);
	    m_server.init_asio(ioSerivce);
	    m_server.set_open_handler(bind(&WebSocketsApi::OnOpen,this, _1));
    	m_server.set_close_handler(bind(&WebSocketsApi::OnClose,this, _1));
	    m_server.listen(wsListenPort);
	    m_server.start_accept();

	}

	void WebSocketsApi::NewSongStatus(const std::string &songName, uint64_t startTimeMs, double speed) {
		json j;
		j["song_is_playing"] = true;
		j["file_name"] = songName;
		j["start_time_millis_since_epoch"] = startTimeMs;
		j["speed"] = speed;
		UpdateLastStatusMsg(j);
	}

	void WebSocketsApi::NoSongPlayingStatus() {
		json j;
		j["song_is_playing"] = false;
		UpdateLastStatusMsg(j);
	}

	void WebSocketsApi::UpdateLastStatusMsg(const json &msgJson) {
		std::string msgJsonStr = msgJson.dump();

		// Test for msg duplication.
		// I do not want to assume something about the clients and how they generate
		// the status messsages, and it is not expensive to do the test.
		// Since the keys order in json message is undefinded, the same logical message can 
		// still produce different string representations (maybe? depends on the library),
		// but that is OK for the use here.
		if(msgJsonStr == m_lastStatusMsg) {
			return;
		}

		m_lastStatusMsg = msgJsonStr;
		SendToAllConnectedClients();			
	}

	void WebSocketsApi::SendToAllConnectedClients() {
		BOOST_FOREACH(const connection_hdl &hdl, m_connections) {
	        m_server.send(hdl, m_lastStatusMsg, websocketpp::frame::opcode::text);
		}			
	}

    void WebSocketsApi::OnOpen(connection_hdl hdl) {
        m_connections.insert(hdl);

        m_server.send(hdl, m_lastStatusMsg, websocketpp::frame::opcode::text);
    }
    
    void WebSocketsApi::OnClose(connection_hdl hdl) {
        m_connections.erase(hdl);
    }

}