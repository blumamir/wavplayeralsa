#ifndef __PLAYER_REQUEST_HPP__
#define __PLAYER_REQUEST_HPP__

#include <string>
#include <cstdint>
#include <sstream>

namespace wavplayeralsa {

	class PlayerRequestIfc {

	public:
		virtual bool NewSongRequest(const std::string &songName, uint64_t startOffsetMs, std::stringstream &outMsg) = 0;
		virtual bool StopPlayRequest(std::stringstream &outMsg) = 0;

	};

}


#endif // __PLAYER_REQUEST_HPP__