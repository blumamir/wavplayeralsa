#include "web_sockets_api.h"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "nlohmann/json.hpp"


using websocketpp::connection_hdl;

using json = nlohmann::json;


namespace wavplayeralsa {

	WebSocketsApi::~WebSocketsApi() {
		if(msg_throttle_timer_ != nullptr) {
			delete msg_throttle_timer_;
			msg_throttle_timer_ = nullptr;
		}
	}

	void WebSocketsApi::Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, uint16_t ws_listen_port) {

		logger_ = logger;
		io_service_ = io_service;
		msg_throttle_timer_ = new boost::asio::deadline_timer(*io_service);

	    server_.clear_error_channels(websocketpp::log::alevel::all);
	    server_.clear_access_channels(websocketpp::log::alevel::all);
	    server_.init_asio(io_service);
	    server_.set_open_handler(websocketpp::lib::bind(&WebSocketsApi::OnOpen,this, websocketpp::lib::placeholders::_1));
    	server_.set_close_handler(websocketpp::lib::bind(&WebSocketsApi::OnClose,this, websocketpp::lib::placeholders::_1));
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

	void WebSocketsApi::NoSongPlayingStatus(const std::string &file_id) {
		json j;
		j["song_is_playing"] = false;
		j["stopped_file_id"] = file_id;
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
		if(!has_active_timer_) {
			msg_throttle_timer_->expires_from_now(boost::posix_time::milliseconds(THROTTLE_WAIT_TIME_MS));
			msg_throttle_timer_->async_wait(boost::bind(&WebSocketsApi::SendToAllConnectedClients, this, _1));			
			has_active_timer_ = true;
		}
	}

	void WebSocketsApi::SendToAllConnectedClients(const boost::system::error_code &e) {
		if(e) {
			return;
		}

		logger_->info("new status message: {}. will send to all {} connected clients", last_status_msg_, connections_.size());
		BOOST_FOREACH(const connection_hdl &hdl, connections_) {
	        server_.send(hdl, last_status_msg_, websocketpp::frame::opcode::text);
		}
		has_active_timer_ = false;
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



