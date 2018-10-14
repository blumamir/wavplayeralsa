#ifndef __POSITION_REPORTER_H__
#define __POSITION_REPORTER_H__

#include <string>
#include <netinet/in.h>

namespace wavplayeralsa {


	class PositionReporter {

	public:
		PositionReporter();
		void sendNewPosition(const std::string &filename, unsigned int currPosition);
		void sendNoTracksPlaying();

	private:
		int fd;
		struct sockaddr_in addr;
		struct in_addr localInterface;
	};

}


#endif //__POSITION_REPORTER_H__