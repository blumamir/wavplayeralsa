#ifndef WAVPLAYERALSA_MQTT_API_H_
#define WAVPLAYERALSA_MQTT_API_H_

#include <cstdint>

#include <boost/asio.hpp>

#include "spdlog/spdlog.h"
#define MQTT_NO_TLS
#include "mqtt_cpp/mqtt_client_cpp.hpp"
#include "nlohmann/json.hpp"

#include "player_actions_ifc.h"
#include "player_events_ifc.h"

#ifndef CURRENT_SONG_TOPIC
#define CURRENT_SONG_TOPIC "current-song"
#endif // CURRENT_SONG_TOPIC

namespace wavplayeralsa {

	class MqttApi : public PlayerEventsIfc {

	public:
		void Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, PlayerActionsIfc *player_action_callback, const std::string &mqtt_host, uint16_t mqtt_port);

	public:
		void NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed);
		void NoSongPlayingStatus();

	private:
		void UpdateLastStatusMsg(const nlohmann::json &msgJson);

	private:
		// outside configurartion
		PlayerActionsIfc *player_action_callback_;
		std::shared_ptr<spdlog::logger> logger_;

	private:
		std::shared_ptr<mqtt::sync_client<mqtt::tcp_endpoint<mqtt::as::ip::tcp::socket, mqtt::as::io_service::strand>>> mqtt_client_;

		std::string last_status_msg_;
	};

}


#endif // WAVPLAYERALSA_MQTT_API_H_
