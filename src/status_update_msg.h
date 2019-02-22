#ifndef __STATUS_UPDATE_MSG_H__
#define __STATUS_UPDATE_MSG_H__



#include <string>
#include <stdint.h>


namespace wavplayeralsa {

	class StatusUpdateMsg {

	public:

		virtual void NewSongStatus(const std::string &songName, uint64_t startTimeMs, double speed) = 0;
		virtual void NoSongPlayingStatus() = 0;


	};


}

#endif // __STATUS_UPDATE_MSG_H__