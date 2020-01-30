#ifndef WAVPLAYERALSA_AUDIO_FILES_MANAGER_H_
#define WAVPLAYERALSA_AUDIO_FILES_MANAGER_H_

#include <string>
#include <cstdint>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include "spdlog/spdlog.h"

#include "player_actions_ifc.h"

namespace wavplayeralsa {

	class AudioFilesManager : 
		public wavplayeralsa::PlayerFilesActionsIfc
	{

	public:

		void Initialize(const std::string &wav_dir);

	public:
		// wavplayeralsa::PlayerFilesActionsIfc
		std::list<std::string> QueryFiles();

	private:
		boost::filesystem::path wav_dir_;

	};



}



#endif // WAVPLAYERALSA_AUDIO_FILES_MANAGER_H_