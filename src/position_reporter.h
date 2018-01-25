#ifndef __POSITION_REPORTER_H__
#define __POSITION_REPORTER_H__

#include <netinet/in.h>

namespace wavplayeralsa {


	class PositionReporter {

	public:
		PositionReporter();
		void sendNewPosition(unsigned int currPosition);

	private:
		int fd;
		struct sockaddr_in addr;
		struct in_addr localInterface;
	};

}


#endif //__POSITION_REPORTER_H__