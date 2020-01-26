#include "mqtt_api.h"

#include <iostream>
#include <boost/date_time/time_duration.hpp>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

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

		json j;
		j["song_is_playing"] = false;
		last_status_msg_ = j.dump();

		const char *mqtt_client_id = "wavplayeralsa";
		logger_->info("creating mqtt connection to host {} on port {} with client id {}", mqtt_host, mqtt_port, mqtt_client_id);
		logger_->info("will publish current song updates on topic {}", CURRENT_SONG_TOPIC);

    	mqtt_client_ = mqtt::make_sync_client(io_service_, mqtt_host, mqtt_port);
        mqtt_client_->set_client_id(mqtt_client_id);
	    mqtt_client_->set_clean_session(true);

    	mqtt_client_->set_error_handler(
			[this]
			(boost::system::error_code ec) {
				logger_->error("client disconnected from mqtt server. will try reconnect in {} ms", reconnect_wait_ms);
				reconnect_timer_.expires_from_now(boost::posix_time::milliseconds(reconnect_wait_ms));
				reconnect_timer_.async_wait(
				    [this] 
					(boost::system::error_code ec) {
						if (ec != boost::asio::error::operation_aborted) {
                    		mqtt_client_->connect();
						}
                	});
			});

    	mqtt_client_->set_close_handler(
			[this]
			() {
				logger_->error("client connection to mqtt server is closed");
			});

	    mqtt_client_->set_connack_handler(
	        [this]
	        (bool sp, std::uint8_t connack_return_code){
	        	logger_->info("connack handler called. clean session: {}. coonack rerturn code: {}", sp, mqtt::connect_return_code_to_str(connack_return_code));
				this->mqtt_client_->publish_exactly_once(CURRENT_SONG_TOPIC, last_status_msg_, true);
	            return true;
	        });	    

	    mqtt_client_->connect();

	}

	void MqttApi::NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed) 
	{
		json j;
		j["song_is_playing"] = true;
		j["file_id"] = file_id;
		j["start_time_millis_since_epoch"] = start_time_millis_since_epoch;
		j["speed"] = speed;
		UpdateLastStatusMsg(j);
	}

	void MqttApi::NoSongPlayingStatus(const std::string &file_id)
	{
		json j;
		j["song_is_playing"] = false;
		j["stopped_file_id"] = file_id;
		UpdateLastStatusMsg(j);
	}

	void MqttApi::UpdateLastStatusMsg(const json &msgJson)
	{
		const std::string msg_json_str = msgJson.dump();

		if(msg_json_str == last_status_msg_) {
			return;
		}

		last_status_msg_ = msg_json_str;		
	    this->mqtt_client_->publish_exactly_once(CURRENT_SONG_TOPIC, last_status_msg_, true);
	}

}