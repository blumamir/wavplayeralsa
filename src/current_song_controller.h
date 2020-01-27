#ifndef CURRENT_SONG_CONTROLLER_H_
#define CURRENT_SONG_CONTROLLER_H_

#include <string>
#include <boost/asio/deadline_timer.hpp>
#include "nlohmann/json_fwd.hpp"

#include <player_events_ifc.h>

#include <mqtt_api.h>
#include <web_sockets_api.h>

using json = nlohmann::json;

namespace wavplayeralsa {

    class CurrentSongController : public PlayerEventsIfc
    {

    public:
        CurrentSongController(boost::asio::io_service &io_service, MqttApi *mqtt_service, WebSocketsApi *ws_service);

    public:
        void NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed);
        void NoSongPlayingStatus(const std::string &file_id);

    private:
        void UpdateLastStatusMsg(const json &msgJson);
        void ReportCurrentSongToServices(const boost::system::error_code& error);

    private:
        MqttApi *mqtt_service_;
        WebSocketsApi *ws_service_;

    private:
    	std::string last_status_msg_;

    private:
        // throttling issues:
        const int THROTTLE_WAIT_TIME_MS = 50;
        boost::asio::deadline_timer throttle_timer_;
        bool throttle_timer_set_ = false;

    };
}

#endif // CURRENT_SONG_CONTROLLER_H_
