#ifndef WAVPLAYERALSA_MQTT_API_H_
#define WAVPLAYERALSA_MQTT_API_H_

#include <cstdint>

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>

#include "spdlog/spdlog.h"
#define MQTT_NO_TLS
#include "mqtt_cpp/mqtt_client_cpp.hpp"

#include "player_actions_ifc.h"

#ifndef CURRENT_SONG_TOPIC
#define CURRENT_SONG_TOPIC "current-song"
#endif // CURRENT_SONG_TOPIC

namespace wavplayeralsa {

	class MqttApi {

	public:
		MqttApi(boost::asio::io_service &io_service);
		void Initialize(std::shared_ptr<spdlog::logger> logger, PlayerActionsIfc *player_action_callback, const std::string &mqtt_host, uint16_t mqtt_port);

	public:
		void ReportCurrentSong(const std::string &json_str);

	private:
		const int reconnect_wait_ms = 2000;

	private:
		// outside services
		PlayerActionsIfc *player_action_callback_;
		std::shared_ptr<spdlog::logger> logger_;
		boost::asio::io_service &io_service_;

	private:
		std::shared_ptr<mqtt::sync_client<mqtt::tcp_endpoint<mqtt::as::ip::tcp::socket, mqtt::as::io_service::strand>>> mqtt_client_ = nullptr;
		boost::asio::deadline_timer reconnect_timer_;

		std::string last_status_msg_;
	};

}


#endif // WAVPLAYERALSA_MQTT_API_H_
