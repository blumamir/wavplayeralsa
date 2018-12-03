#include "position_reporter.h"

#include "generated/position_report.pb.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <random>


namespace wavplayeralsa {

	PositionReporter::PositionReporter() {
	    std::default_random_engine eng((std::random_device())());
	    std::uniform_int_distribution<uint32_t> idis(0, std::numeric_limits<uint32_t>::max());
	    this->guid = idis(eng);
	    std::cout << "guid for this player is: " << this->guid << std::endl;
	}

	void PositionReporter::initialize(uint16_t broadcastPort) {
		m_broadcastPort = broadcastPort;

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
		memset(&addr,0,sizeof(addr));
		addr.sin_family=AF_INET;
		addr.sin_addr.s_addr=htonl(INADDR_BROADCAST);
		addr.sin_port=htons(m_broadcastPort);

	}

	void PositionReporter::sendNewPosition(const std::string &filename, unsigned int currPosition, uint32_t positionCookie) {

		PositionReportMsg prProto;
		PositionReportMsg::Song *s = prProto.add_songs();
		s->set_song_name(filename);
		s->set_position_in_ms(currPosition);
		s->set_volume(1.0);
		s->set_position_cookie(positionCookie);
		s->set_guid(this->guid);
		std::string outputBuf;
		prProto.SerializeToString(&outputBuf);

		//std::cout << "current position: " << currPosition << " (ms) and " << currPosition / 1000.0 / 60.0 << " (minutes)" << std::endl;

		ssize_t bytesSent = sendto(fd,outputBuf.c_str(),outputBuf.length(),0,(struct sockaddr *) &addr, sizeof(addr));
		if(bytesSent > 0) {
			if(!hasNetworkForSend) {
				std::cout << "detected network again. will now send position reports on socket" << std::endl;
				hasNetworkForSend = true;
			}
		}
		else {
			if(hasNetworkForSend) {
				perror("position report send on network failed. will recover when network return");
				hasNetworkForSend = false;							
			}
		}
	}

	void PositionReporter::sendNoTracksPlaying() {
		PositionReportMsg prProto;
		std::string outputBuf;
		prProto.SerializeToString(&outputBuf);		
		if (sendto(fd,outputBuf.c_str(),outputBuf.length(),0,(struct sockaddr *) &addr,
			sizeof(addr)) < 0) {
			perror("sendto");
			exit(1);
		}
	}

}
