#include "mqtt_api.h"

#include <iostream>

using json = nlohmann::json;

namespace wavplayeralsa {

	void MqttApi::Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, PlayerActionsIfc *player_action_callback, const std::string &mqtt_host, uint16_t mqtt_port) {

		// set class members
		player_action_callback_ = player_action_callback;
		logger_ = logger;

    	mqtt_client_ = mqtt::make_sync_client(*io_service, mqtt_host, mqtt_port);
        mqtt_client_->set_client_id("cid1");
	    mqtt_client_->set_clean_session(true);

	    mqtt_client_->set_connack_handler(
	        // [&c, &pid_sub1, &pid_sub2]
	        [this]
	        (bool sp, std::uint8_t connack_return_code){
	            std::cout << "Connack handler called" << std::endl;
	            std::cout << "Clean Session: " << std::boolalpha << sp << std::endl;
	            std::cout << "Connack Return Code: "
	                      << mqtt::connect_return_code_to_str(connack_return_code) << std::endl;
	            this->mqtt_client_->publish_exactly_once("test", "test2_2");
	            // if (connack_return_code == mqtt::connect_return_code::accepted) {
	            //     pid_sub1 = c->subscribe("mqtt_client_cpp/topic1", mqtt::qos::at_most_once);
	            //     pid_sub2 = c->subscribe("mqtt_client_cpp/topic2_1", mqtt::qos::at_least_once,
	            //                            "mqtt_client_cpp/topic2_2", mqtt::qos::exactly_once);
	            // }
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
	    this->mqtt_client_->publish_exactly_once("current-song", last_status_msg_);
	}

}