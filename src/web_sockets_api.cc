#include "web_sockets_api.h"

#include <boost/foreach.hpp>
#include <boost/bind.hpp>

using websocketpp::connection_hdl;

namespace wavplayeralsa {

	WebSocketsApi::WebSocketsApi()
	{
		
	}

	void WebSocketsApi::Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, uint16_t ws_listen_port) {

		logger_ = logger;

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

		initialized = true;
	}

	void WebSocketsApi::ReportCurrentSong(const std::string &json_str) 
	{
		last_status_msg_ = json_str;

		if(!initialized)
			return;

		logger_->info("new status message: {}. will send to all {} connected clients", last_status_msg_, connections_.size());
		BOOST_FOREACH(const connection_hdl &hdl, connections_) {
	        server_.send(hdl, last_status_msg_, websocketpp::frame::opcode::text);
		}
	}

    void WebSocketsApi::OnOpen(connection_hdl hdl) 
	{
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



