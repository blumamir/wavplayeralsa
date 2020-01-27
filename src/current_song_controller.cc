#include <current_song_controller.h>

#include <iostream>
#include <boost/bind.hpp>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace wavplayeralsa 
{

    CurrentSongController::CurrentSongController(boost::asio::io_service &io_service, MqttApi *mqtt_service, WebSocketsApi *ws_service)
        : throttle_timer_(io_service)
    {
        mqtt_service_ = mqtt_service;
        ws_service_ = ws_service;

        json j;
		j["song_is_playing"] = false;
		UpdateLastStatusMsg(j);
    }

    void CurrentSongController::NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed)
    {
		json j;
		j["song_is_playing"] = true;
		j["file_id"] = file_id;
		j["start_time_millis_since_epoch"] = start_time_millis_since_epoch;
		j["speed"] = speed;  
        UpdateLastStatusMsg(j);
    }

    void CurrentSongController::NoSongPlayingStatus(const std::string &file_id)       
    {
		json j;
		j["song_is_playing"] = false;
		j["stopped_file_id"] = file_id;
        UpdateLastStatusMsg(j);
    }

	void CurrentSongController::UpdateLastStatusMsg(const json &msgJson)
	{
		const std::string msg_json_str = msgJson.dump();

		if(msg_json_str == last_status_msg_) {
			return;
		}

		last_status_msg_ = msg_json_str;

        if(!throttle_timer_set_) {
            throttle_timer_.expires_from_now(boost::posix_time::milliseconds(THROTTLE_WAIT_TIME_MS));
            throttle_timer_.async_wait(boost::bind(&CurrentSongController::ReportCurrentSongToServices, this, _1));			
            throttle_timer_set_ = true;
        }
	}

    void CurrentSongController::ReportCurrentSongToServices(const boost::system::error_code& error)
    {
        throttle_timer_set_ = false;
        if(error)
            return;

        mqtt_service_->ReportCurrentSong(last_status_msg_);
        ws_service_->ReportCurrentSong(last_status_msg_);        
    }

}


