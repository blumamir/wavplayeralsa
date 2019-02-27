#include "web_sockets_api.h"

#include <boost/foreach.hpp>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::connection_hdl;

using json = nlohmann::json;


namespace wavplayeralsa {


	void WebSocketsApi::Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, uint16_t ws_listen_port) {

		logger_ = logger;

	    server_.clear_error_channels(websocketpp::log::alevel::all);
	    server_.clear_access_channels(websocketpp::log::alevel::all);
	    server_.init_asio(io_service);
	    server_.set_open_handler(bind(&WebSocketsApi::OnOpen,this, _1));
    	server_.set_close_handler(bind(&WebSocketsApi::OnClose,this, _1));
    	try {
	    	server_.listen(ws_listen_port);
	    }
	    catch(const websocketpp::exception & e) {
	    	std::stringstream err_msg;
	    	err_msg << "web socket server listen on port " << ws_listen_port << " failed, probably not able to bind to port. error msg: " << e.what();
	    	throw std::runtime_error(err_msg.str());
	    }
	    server_.start_accept();

	  	logger_->info("web sockets server started on port {}", ws_listen_port);
	}

	void WebSocketsApi::NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed) {
		json j;
		j["song_is_playing"] = true;
		j["file_id"] = file_id;
		j["start_time_millis_since_epoch"] = start_time_millis_since_epoch;
		j["speed"] = speed;
		UpdateLastStatusMsg(j);
	}

	void WebSocketsApi::NoSongPlayingStatus() {
		json j;
		j["song_is_playing"] = false;
		UpdateLastStatusMsg(j);
	}

	void WebSocketsApi::UpdateLastStatusMsg(const json &msgJson) {
		std::string msg_json_str = msgJson.dump();

		// Test for msg duplication.
		// this is just optimization, and may not cover all cases (json keys are not ordered),
		// but its cheap and easy to test.
		if(msg_json_str == last_status_msg_) {
			return;
		}

		last_status_msg_ = msg_json_str;
		logger_->info("new status message: {}. will send to all {} connected clients", last_status_msg_, connections_.size());
		SendToAllConnectedClients();			
	}

	void WebSocketsApi::SendToAllConnectedClients() {
		BOOST_FOREACH(const connection_hdl &hdl, connections_) {
	        server_.send(hdl, last_status_msg_, websocketpp::frame::opcode::text);
		}			
	}

    void WebSocketsApi::OnOpen(connection_hdl hdl) {
        connections_.insert(hdl);

		const auto con = server_.get_con_from_hdl(hdl);
		const boost::asio::ip::address socket_address = con->get_raw_socket().remote_endpoint().address();
		logger_->info("new web socket connection from ip {}, ptr for close reference: {}", socket_address.to_string(), hdl.lock().get());
        server_.send(hdl, last_status_msg_, websocketpp::frame::opcode::text);
    }
    
    void WebSocketsApi::OnClose(connection_hdl hdl) {
		logger_->info("connection closed. ptr for closed connection: {}", hdl.lock().get());
        connections_.erase(hdl);
    }

}