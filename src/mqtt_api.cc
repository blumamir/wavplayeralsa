#include "mqtt_api.h"

#include <iostream>
#include <functional>
#include <boost/date_time/time_duration.hpp>

namespace wavplayeralsa {

	MqttApi::MqttApi(boost::asio::io_service &io_service) :
		io_service_(io_service),
		reconnect_timer_(io_service)
	{

	}

	void MqttApi::Initialize(std::shared_ptr<spdlog::logger> logger, PlayerActionsIfc *player_action_callback, const std::string &mqtt_host, uint16_t mqtt_port) {

		// set class members
		player_action_callback_ = player_action_callback;
		logger_ = logger;

		const char *mqtt_client_id = "wavplayeralsa";
		logger_->info("creating mqtt connection to host {} on port {} with client id {}", mqtt_host, mqtt_port, mqtt_client_id);
		logger_->info("will publish current song updates on topic {}", CURRENT_SONG_TOPIC);

    	mqtt_client_ = mqtt::make_sync_client(io_service_, mqtt_host, mqtt_port);
        mqtt_client_->set_client_id(mqtt_client_id);
	    mqtt_client_->set_clean_session(true);

    	mqtt_client_->set_error_handler(std::bind(&MqttApi::OnError, this, std::placeholders::_1));
    	mqtt_client_->set_close_handler(std::bind(&MqttApi::OnClose, this));
	    mqtt_client_->set_connack_handler(std::bind(&MqttApi::OnConnAck, this, std::placeholders::_1, std::placeholders::_2));

	    mqtt_client_->connect();

	}

	void MqttApi::ReportCurrentSong(const std::string &json_str)
	{
		last_status_msg_ = json_str;
		PublishCurrentSong();
	}

	void MqttApi::OnError(boost::system::error_code ec)
	{
		logger_->error("client disconnected from mqtt server. will try reconnect in {} ms", RECONNECT_WAIT_MS);
		reconnect_timer_.expires_from_now(boost::posix_time::milliseconds(RECONNECT_WAIT_MS));
		reconnect_timer_.async_wait(
			[this] 
			(boost::system::error_code ec) {
				if (ec != boost::asio::error::operation_aborted) {
					mqtt_client_->connect();
				}
			});
	}

	void MqttApi::OnClose()
	{
		logger_->error("client connection to mqtt server is closed");
	}

	bool MqttApi::OnConnAck(bool session_present, std::uint8_t connack_return_code)
	{
		logger_->info("connack handler called. clean session: {}. coonack rerturn code: {}", session_present, mqtt::connect_return_code_to_str(connack_return_code));
		PublishCurrentSong();
		return true;
	}

	void MqttApi::PublishCurrentSong()
	{
		if(!this->mqtt_client_)
			return;

		if(last_status_msg_.empty())
			return;

		this->mqtt_client_->publish_exactly_once(CURRENT_SONG_TOPIC, last_status_msg_, true);
	}

}