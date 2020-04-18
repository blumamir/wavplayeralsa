#ifndef WAVPLAYERALSA_CONFIG_SERVICE_H__
#define WAVPLAYERALSA_CONFIG_SERVICE_H__

#include <string>
#include <cstdint>
#include <memory>
#include "spdlog/spdlog.h"

namespace wavplayeralsa
{

    class ConfigService
    {

    public:
        ConfigService();
        bool InitFromCmdArguments(int argc, char *argv[]);
        void LogConfig(std::shared_ptr<spdlog::logger> logger) const;

    private:
        void LoadConfigFile(const std::string &path); 
        void SetParamFromFile(const std::string &param_name, const std::string &param_value);

    public:
        bool SaveLogsToFile() const { return !log_dir_.empty(); }
        bool UseMqtt() const { return !mqtt_host_.empty(); }
        bool HasConfigFile() const { return !config_file_.empty(); }

    public:
        std::string GetLogDir() const { return log_dir_; }
        std::string GetInitialFile() const { return initial_file_; }
        uint16_t GetWsListenPort() const { return ws_listen_port_; }
        uint16_t GetHttpListenPort() const { return http_listen_port_; }
        std::string GetMqttHost() const { return mqtt_host_; }
        uint16_t GetMqttPort() const { return mqtt_port_; }
        std::string GetWavDir() const { return wav_dir_; }
        std::string GetAudioDevice() const { return audio_device_; }

    private:
        std::string config_file_;

    private:
        std::string log_dir_;
        std::string initial_file_;
        uint16_t ws_listen_port_ = 9002;
        uint16_t http_listen_port_ = 8080;
        std::string mqtt_host_;
        uint16_t mqtt_port_ = 1883;
        std::string wav_dir_;
        std::string audio_device_ = "default";

    };
}

#endif // WAVPLAYERALSA_CONFIG_SERVICE_H__
