#include <current_song_controller.h>

#include <iostream>
#include <boost/bind.hpp>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace wavplayeralsa 
{

    CurrentSongController::CurrentSongController(
        boost::asio::io_service &io_service, 
        MqttApi *mqtt_service, 
        WebSocketsApi *ws_service,
        AlsaFramesTransfer *alsa_service)
            : 
        ios_(io_service), 
        mqtt_service_(mqtt_service),
        ws_service_(ws_service),
        alsa_service_(alsa_service),
        throttle_timer_(io_service)
    {
        json j;
		j["song_is_playing"] = false;
		UpdateLastStatusMsg(j);
    }

    void CurrentSongController::Initialize(const std::string &player_uuid, const std::string &wav_dir)
    {
        player_uuid_ = player_uuid;
        wav_dir_ = boost::filesystem::path(wav_dir);
    }

    void CurrentSongController::NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed)
    {
		json j;
		j["song_is_playing"] = true;
		j["file_id"] = file_id;
		j["start_time_millis_since_epoch"] = start_time_millis_since_epoch;
		j["speed"] = speed;

        ios_.post(std::bind(&CurrentSongController::UpdateLastStatusMsg, this, j));
    }

    void CurrentSongController::NoSongPlayingStatus(const std::string &file_id)       
    {
		json j;
		j["song_is_playing"] = false;
		j["stopped_file_id"] = file_id;
        
        ios_.post(std::bind(&CurrentSongController::UpdateLastStatusMsg, this, j));
    }

	bool CurrentSongController::NewSongRequest(const std::string &file_id, uint64_t start_offset_ms, std::stringstream &out_msg) {

		if(file_id == alsa_service_->GetFileId()) {
			out_msg << "changed position of the current file '" << file_id << "'. new position in ms is: " << start_offset_ms << std::endl;
		}
		else {

			// create the canonical full path of the file to play
			boost::filesystem::path songPathInWavDir(file_id);
			boost::filesystem::path songFullPath = wav_dir_ / songPathInWavDir;
			std::string canonicalFullPath;
			try {
			 	canonicalFullPath = boost::filesystem::canonical(songFullPath).string();
			}
			catch (const std::exception &e) {
				out_msg << "loading new audio file '" << file_id << "' failed. error: " << e.what();
				return false;
			}

			try {
				const std::string prev_file = alsa_service_->GetFileId();
				bool prev_file_was_playing = alsa_service_->LoadNewFile(canonicalFullPath, file_id);

				// message printing
				const int SECONDS_PER_HOUR = (60 * 60);
				uint64_t start_offset_sec = start_offset_ms / 1000;
				uint64_t hours = start_offset_sec / SECONDS_PER_HOUR;
				start_offset_sec = start_offset_sec - hours * SECONDS_PER_HOUR;
				uint64_t minutes = start_offset_sec / 60;
				uint64_t seconds = start_offset_sec % 60;
				if(prev_file_was_playing && !prev_file.empty()) {
					out_msg << "audio file successfully changed from '" << prev_file << "' to '" << file_id << "' and will be played ";
				}
				else {
					out_msg << "will play audio file '" << file_id << "' ";
				}
				out_msg << "starting at position " << start_offset_ms << " ms " <<
					"(" << hours << ":" << 
					std::setfill('0') << std::setw(2) << minutes << ":" << 
					std::setfill('0') << std::setw(2) << seconds << ")";
			}
			catch(const std::runtime_error &e) {
				out_msg << "loading new audio file '" << file_id << "' failed. currently no audio file is loaded in the player and it is not playing. " <<
					"reason for failure: " << e.what();
				return false;
			}
		}

		alsa_service_->StartPlay(start_offset_ms);
		return true;
	}

	bool CurrentSongController::StopPlayRequest(std::stringstream &out_msg) {

		bool was_playing = false;
		try {
			was_playing = alsa_service_->Stop();
		}
		catch(const std::runtime_error &e) {
			out_msg << "Unable to stop current audio file successfully, error: " << e.what();
			return false;
		}

		const std::string &current_file_id = alsa_service_->GetFileId();
		if(current_file_id.empty() || !was_playing) {
			out_msg << "no audio file is being played, so stop had no effect";			
		}
		else {
			out_msg << "current audio file '" << current_file_id << "' stopped playing";				
		}
		return true;
	}

	void CurrentSongController::UpdateLastStatusMsg(const json &alsa_data)
	{
        json full_msg(alsa_data);
        full_msg["uuid"] = player_uuid_;

		const std::string msg_json_str = full_msg.dump();

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


