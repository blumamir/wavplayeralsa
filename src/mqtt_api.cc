#include "mqtt_api.h"

#include <iostream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace wavplayeralsa {

	void MqttApi::Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, PlayerActionsIfc *player_action_callback, const std::string &mqtt_host, uint16_t mqtt_port) {

		// set class members
		player_action_callback_ = player_action_callback;
		logger_ = logger;

		const char *mqtt_client_id = "wavplayeralsa";
		logger_->info("creating mqtt connection to host {} on port {} with client id {}", mqtt_host, mqtt_port, mqtt_client_id);
		logger_->info("will publish current song updates on topic {}", CURRENT_SONG_TOPIC);

    	mqtt_client_ = mqtt::make_sync_client(*io_service, mqtt_host, mqtt_port);
        mqtt_client_->set_client_id(mqtt_client_id);
	    mqtt_client_->set_clean_session(true);

	    mqtt_client_->set_connack_handler(
	        // [&c, &pid_sub1, &pid_sub2]
	        [this]
	        (bool sp, std::uint8_t connack_return_code){
	        	logger_->info("connack handler called. clean session: {}. coonack rerturn code: {}", sp, mqtt::connect_return_code_to_str(connack_return_code));
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

	void MqttApi::NoSongPlayingStatus()
	{
		json j;
		j["song_is_playing"] = false;
		UpdateLastStatusMsg(j);
	}

	void MqttApi::UpdateLastStatusMsg(const json &msgJson)
	{
		const std::string msg_json_str = msgJson.dump();

		if(msg_json_str == last_status_msg_) {
			return;
		}

		last_status_msg_ = msg_json_str;		
	    this->mqtt_client_->publish_exactly_once(CURRENT_SONG_TOPIC, last_status_msg_);
	}

}