#ifndef __POSITION_REPORTER_H__
#define __POSITION_REPORTER_H__

#include <string>
#include <netinet/in.h>

namespace wavplayeralsa {


	class PositionReporter {

	public:
		PositionReporter();
		void initialize(uint16_t broadcastPort);
		void sendNewPosition(const std::string &filename, unsigned int currPosition);
		void sendNoTracksPlaying();

	private:
		int fd;
		uint16_t m_broadcastPort;
		struct sockaddr_in addr;
		struct in_addr localInterface;
	};

}


#endif //__POSITION_REPORTER_H__