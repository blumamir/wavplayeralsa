#ifndef WAVPLAYERALSA_PLAYER_ACTIONS_IFC_H_
#define WAVPLAYERALSA_PLAYER_ACTIONS_IFC_H_

#include <string>
#include <cstdint>
#include <sstream>

/*
This interface describe the actions that can be performed on the player externally
*/

namespace wavplayeralsa {

	class PlayerActionsIfc {

	public:
		virtual bool NewSongRequest(const std::string &songName, uint64_t startOffsetMs, std::stringstream &outMsg) = 0;
		virtual bool StopPlayRequest(std::stringstream &outMsg) = 0;

	};

}


#endif // WAVPLAYERALSA_PLAYER_ACTIONS_IFC_H_