#include <status_reporter_ifc.hpp>

#include <set>

#include <boost/foreach.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <nlohmann/json.hpp>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::connection_hdl;

typedef websocketpp::server<websocketpp::config::asio> ws_echo_server;

using json = nlohmann::json;


// void on_message(ws_echo_server* s, websocketpp::connection_hdl hdl, ws_echo_server::message_ptr msg) {
//     std::cout << "on_message called with hdl: " << hdl.lock().get()
//               << " and message: " << msg->get_payload()
//               << std::endl;

//     // check for a special command to instruct the server to stop listening so
//     // it can be cleanly exited.
//     if (msg->get_payload() == "stop-listening") {
//         s->stop_listening();
//         return;
//     }

//     try {
//         s->send(hdl, msg->get_payload(), msg->get_opcode());
//     } catch (websocketpp::exception const & e) {
//         std::cout << "Echo failed because: "
//                   << "(" << e.what() << ")" << std::endl;
//     }
// }



namespace wavplayeralsa {


	class StatusReporter : public StatusReporterIfc {

	public:

		void Configure(boost::asio::io_service *ioSerivce, uint16_t wsListenPort) {

		    m_server.clear_error_channels(websocketpp::log::alevel::all);
		    m_server.clear_access_channels(websocketpp::log::alevel::all);
		    m_server.init_asio(ioSerivce);
		    m_server.set_open_handler(bind(&StatusReporter::on_open,this,_1));
        	m_server.set_close_handler(bind(&StatusReporter::on_close,this,_1));
		    //ws_server.set_message_handler(bind(&on_message,&ws_server,::_1,::_2));
		    m_server.listen(wsListenPort);
		    m_server.start_accept();

		}

	public:

		void NewSongStatus(const std::string &songName, uint64_t startTimeMs, double speed) {
			json j;
			j["song_is_playing"] = true;
			j["file_name"] = songName;
			j["start_time_millis_since_epoch"] = startTimeMs;
			j["speed"] = speed;
			UpdateLastStatusMsg(j);
		}

		void NoSongPlayingStatus() {
			json j;
			j["song_is_playing"] = false;
			UpdateLastStatusMsg(j);
		}

		void UpdateLastStatusMsg(const json &msgJson) {
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

	private:

		void SendToAllConnectedClients() {
			BOOST_FOREACH(const connection_hdl &hdl, m_connections) {
		        m_server.send(hdl, m_lastStatusMsg, websocketpp::frame::opcode::text);
			}			
		}

	    void on_open(connection_hdl hdl) {
	        m_connections.insert(hdl);

	        m_server.send(hdl, m_lastStatusMsg, websocketpp::frame::opcode::text);
	    }
	    
	    void on_close(connection_hdl hdl) {
	        m_connections.erase(hdl);
	    }


	private:

		ws_echo_server m_server;

		typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;
		con_list m_connections;

		std::string m_lastStatusMsg;

	};

	StatusReporterIfc* CreateStatusReporter() {
		return new StatusReporter();
	}

}