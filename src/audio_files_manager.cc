#include "audio_files_manager.h"

#include <iomanip>

#include <boost/foreach.hpp>

namespace wavplayeralsa {

	void AudioFilesManager::Initialize(
			AlsaFramesTransfer *alsa_frames_transfer,
			const std::string &wav_dir)
	{
		wav_dir_ = boost::filesystem::path(wav_dir);
		alsa_frames_transfer_ = alsa_frames_transfer;
	}

	bool AudioFilesManager::NewSongRequest(const std::string &file_id, uint64_t start_offset_ms, std::stringstream &out_msg) {

		if(file_id == alsa_frames_transfer_->GetFileId()) {
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
				const std::string prev_file = alsa_frames_transfer_->GetFileId();
				bool prev_file_was_playing = alsa_frames_transfer_->LoadNewFile(canonicalFullPath, file_id);

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

		alsa_frames_transfer_->StartPlay(start_offset_ms);
		return true;
	}

	bool AudioFilesManager::StopPlayRequest(std::stringstream &out_msg) {

		bool was_playing = false;
		try {
			was_playing = alsa_frames_transfer_->Stop();
		}
		catch(const std::runtime_error &e) {
			out_msg << "Unable to stop current audio file successfully, error: " << e.what();
			return false;
		}

		const std::string &current_file_id = alsa_frames_transfer_->GetFileId();
		if(current_file_id.empty() || !was_playing) {
			out_msg << "no audio file is being played, so stop had no effect";			
		}
		else {
			out_msg << "current audio file '" << current_file_id << "' stopped playing";				
		}
		return true;
	}

	std::list<std::string> AudioFilesManager::QueryFiles() {

		std::list<std::string> fileIds;

		std::size_t trailing_path_len = wav_dir_.string().length();
		for ( boost::filesystem::recursive_directory_iterator end, dir(wav_dir_); dir != end; ++dir ) {
			if(boost::filesystem::is_regular_file(dir->status())) {
				const boost::filesystem::directory_entry &directory_entry = *dir;
				fileIds.push_back(directory_entry.path().string().substr(trailing_path_len));				
			}
		}
		return fileIds;
	}

}

