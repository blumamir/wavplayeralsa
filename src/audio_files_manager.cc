#include "audio_files_manager.h"

#include <iomanip>

#include <boost/foreach.hpp>

namespace wavplayeralsa {

	void AudioFilesManager::Initialize(const std::string &wav_dir)
	{
		wav_dir_ = boost::filesystem::path(wav_dir);
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

