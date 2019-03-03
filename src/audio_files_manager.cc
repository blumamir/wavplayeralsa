#include "audio_files_manager.h"

namespace wavplayeralsa {

	void AudioFilesManager::Initialize(std::shared_ptr<spdlog::logger> alsa_frames_transfer_logger, 
			boost::asio::io_service *main_io_service, 
			PlayerEventsIfc *player_events_ifc, 
			const std::string &wav_dir, 
			const std::string &audio_device) 
	{
		wav_dir_ = boost::filesystem::path(wav_dir);
		main_io_service_ = main_io_service;
		player_events_ifc_ = player_events_ifc;

		alsa_frames_transfer_.Initialize(alsa_frames_transfer_logger, this, audio_device);
	}

	bool AudioFilesManager::NewSongRequest(const std::string &file_id, uint64_t start_offset_ms, std::stringstream &out_msg) {

		if(file_id == alsa_frames_transfer_.GetFileId()) {
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
				alsa_frames_transfer_.LoadNewFile(canonicalFullPath, file_id);
				out_msg << "song successfully changed to '" << file_id << "'. " <<
						"new audio file will start playing at position " << start_offset_ms << " ms";
			}
			catch(const std::runtime_error &e) {
				out_msg << "loading new audio file '" << file_id << "' failed. currently no audio file is loaded in the player and it is not playing. " <<
					"reason for failure: " << e.what();
				return false;
			}
		}

		alsa_frames_transfer_.StartPlay(start_offset_ms);
		return true;
	}

	bool AudioFilesManager::StopPlayRequest(std::stringstream &out_msg) {
		try {
			alsa_frames_transfer_.Stop();
		}
		catch(const std::runtime_error &e) {
			out_msg << "Unable to stop current song successfully, error: " << e.what();
			return false;
		}
		out_msg << "current song '" << alsa_frames_transfer_.GetFileId() << "' stopped playing";
		return true;
	}

	void AudioFilesManager::NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed) {
		main_io_service_->post(boost::bind(&PlayerEventsIfc::NewSongStatus, player_events_ifc_, file_id, start_time_millis_since_epoch, speed));
	}

	void AudioFilesManager::NoSongPlayingStatus() {
		main_io_service_->post(boost::bind(&PlayerEventsIfc::NoSongPlayingStatus, player_events_ifc_));		
	}	

}

