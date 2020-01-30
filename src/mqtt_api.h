#ifndef WAVPLAYERALSA_MQTT_API_H_
#define WAVPLAYERALSA_MQTT_API_H_

#include <cstdint>

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>

#include "spdlog/spdlog.h"
#define MQTT_NO_TLS
#include "mqtt_cpp/mqtt_client_cpp.hpp"

namespace wavplayeralsa {

	class MqttApi {

	public:
		MqttApi(boost::asio::io_service &io_service);
		void Initialize(
			std::shared_ptr<spdlog::logger> logger, 
			const std::string &mqtt_host, 
			uint16_t mqtt_port);

	public:
		void ReportCurrentSong(const std::string &json_str);

	private:
		void OnError(boost::system::error_code ec);
		void OnClose();
		bool OnConnAck(bool session_present, std::uint8_t connack_return_code);

	private:
		void PublishCurrentSong();

	private:
		const int RECONNECT_WAIT_MS = 2000;
		const char *CURRENT_SONG_TOPIC = "current-song";

	private:
		// outside services
		std::shared_ptr<spdlog::logger> logger_;
		boost::asio::io_service &io_service_;

	private:
		std::shared_ptr<mqtt::sync_client<mqtt::tcp_endpoint<mqtt::as::ip::tcp::socket, mqtt::as::io_service::strand>>> mqtt_client_ = nullptr;
		boost::asio::deadline_timer reconnect_timer_;

		std::string last_status_msg_;
	};

}


#endif // WAVPLAYERALSA_MQTT_API_H_
