#include "position_reporter.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "generated/position_report.pb.h"


#define HELLO_PORT 2001
#define HELLO_GROUP "10.0.0.255"

#include <iostream>

namespace wavplayeralsa {

	PositionReporter::PositionReporter() {

/* create what looks like an ordinary UDP socket */
		if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
			perror("socket");
			exit(1);
		}

		int broadcastEnable = 1;
		int rt = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
		if(rt < 0) {
			perror("broadcast failed");
			exit(1);			
		}

/* set up destination address */
		// memset(&addr,0,sizeof(addr));
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=inet_addr(HELLO_GROUP);
		addr.sin_port=htons(HELLO_PORT);

		/*localInterface.s_addr = inet_addr("10.0.0.103");
		if(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0) {
			perror("setting local interface error");
			exit(1);
		}
		else {
			std::cout << "setting the local interface... OK" << std::endl;
		}*/
	}

	void PositionReporter::sendNewPosition(unsigned int currPosition, const std::string &filename) {

		PositionReport prProto;
		PositionReport::Song *s = prProto.add_songs();
		s->set_song_name(filename);
		s->set_position_in_ms(currPosition);
		s->set_volume(1.0);
		std::string outputBuf;
		prProto.SerializeToString(&outputBuf);

		std::cout << "current position: " << currPosition << " (ms) and " << currPosition / 1000.0 / 60.0 << " (minutes)" << std::endl;

		if (sendto(fd,outputBuf.c_str(),outputBuf.length(),0,(struct sockaddr *) &addr,
			sizeof(addr)) < 0) {
			perror("sendto");
			exit(1);
		}
	}
}
