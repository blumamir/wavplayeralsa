#ifndef WAVPLAYERALSA_AUDIO_FILES_MANAGER_H_
#define WAVPLAYERALSA_AUDIO_FILES_MANAGER_H_

#include <string>
#include <cstdint>
#include <sstream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>

#include "spdlog/spdlog.h"

#include "player_actions_ifc.h"
#include "player_events_ifc.h"
#include "alsa_frames_transfer.h"

namespace wavplayeralsa {

	/*
	This class offloads some of the tasks of the alsa_frames_transfer, so the former one is more simple and thin.
	These tasks are:
	- it allow alsa_frames_transfer to publish PlayerEventsIfc calls on it's own thread, without need to know
		or care about transfering the interface calls to the right clients and io service.
	- it handles all the file_id and audio files manipulations and hands over to alsa_frames_transfer a clean
		and proccessed file name.
	- it handles some of the details involved in PlayerActionsIfc, for example: producing a human readable,
		out_msg containing the action status.
	*/
	class AudioFilesManager : 
		public wavplayeralsa::PlayerActionsIfc,
		public wavplayeralsa::PlayerEventsIfc 
	{

	public:

		void Initialize(std::shared_ptr<spdlog::logger> alsa_frames_transfer_logger, boost::asio::io_service *main_io_service, const std::string &wav_dir, const std::string &audio_device);
		void RegisterPlayerEventsHandler(PlayerEventsIfc *player_events_ifc);

	public:
		// wavplayeralsa::PlayerActionsIfc
		bool NewSongRequest(const std::string &file_id, uint64_t start_offset_ms, std::stringstream &out_msg);
		bool StopPlayRequest(std::stringstream &out_msg);
		std::list<std::string> QueryFiles();

	public:
		// wavplayeralsa::PlayerEventsIfc
		void NewSongStatus(const std::string &file_id, uint64_t start_time_millis_since_epoch, double speed);
		void NoSongPlayingStatus();

	private:
		boost::filesystem::path wav_dir_;

		AlsaFramesTransfer alsa_frames_transfer_;
		
		boost::asio::io_service *main_io_service_;
		std::vector<PlayerEventsIfc *> player_events_ifc_;

	};



}



#endif // WAVPLAYERALSA_AUDIO_FILES_MANAGER_H_