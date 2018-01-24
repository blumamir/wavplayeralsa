
#include "single_file_player.h"
#include <iostream>
#include <sndfile.h>
#include <alsa/asoundlib.h>


int main(int argc, char *argv[]) {

	if (argc <= 1) {
		std::cout << "Usage: " << argv[0] << " file.wav" << std::endl;
		return -1;
	}


	const char *filename = argv[1];

	wavplayeralsa::SingleFilePlayer player;
	player.setFileToPlay(filename);
	try {
		player.initialize();		
	}
	catch(const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "goodbey" << std::endl;
		return -1;
	}
	player.startPlay(0);
	while(true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		unsigned int positionMs = player.getPositionInMs();
		//std::cout << (positionMs / 1000.0) / 60.0 << std::endl;
	}

	return 0;
}


