#ifndef __SINGLE_FILE_PLAYER_H__
#define __SINGLE_FILE_PLAYER_H__

#include <string>
#include <thread>
#include <atomic>
#include <cstdint>

#include <alsa/asoundlib.h>
#include <sndfile.hh>

#include "status_update_msg.h"

/*
Detailed explanations on how this module operate.
This it the part that actually speak with ALSA, and send the audio data that should be played by the audio hardware.
How do we send audio to be played? 
	* we use ALSA library API, 
	-> which speak with ALSA kernel API
	-> which speaks with the audio driver
	-> which send the audio data to the sound card
	-> which send the audio data to the hardware
	-> which produces the actual sound that we hear

ALSA (Advanced Linux Sound Architecture) is the low level software framework that handles sound in linux.
Why did I choose to use ALSA? first I discuss what was the goals I wanted to achive.

Objectives:
This application was written to handle the job of running LED shows, synchronized to music.
Like watching a movie, it is extremely important for a good synchronization of the visual and the audio.
If a gap of more than few tens of milliseconds is present, the user expirence is severily damaged.
So the first objective of the application is to achive very accurate offset in the played audio (few ms at most).
The other issue is reliability. The application should be able to efficiently and reliabliy handle 
reasonable file formats and data representaions on many platforms.
The third issue is to allow other parts of the software stack to be written in a "microservice" approch, 
in any programming language, and in any architecture (not necessarely as a single process or on the same machine).

Why alsa then?
Alsa is the low level part of sound handling in linux, so using it would allow for the most 
reliable, efficient, and precise handling of audio. 
Other higher-level libraries that are built on top of ALSA, can introduce problems which we've seen:
- incorrect audio offset reporting
- non supporting format (like Java's javax.sound.sampled.AudioSystem which had no support for mono 
	files on raspberry pi, causing waste of hours of debuging, and prone to error limitation to
	convert all used files to stereo).
- efficient use of the resources - working with c++ and removing other layers that might add "surprises"
	and have different objectives then us.

So how do we speak with ALSA?

First, we open the raw wav file with the libsndfile library, which we use to:
1. read file metadata from the header:
	1.1 frame rate - how many frames per seconds should be played
	1.2 channels - how many samples are in each frame
	1.3 sample size in bytes - how many bytes are used to save each sample
	1.4 sample type - how is each sampled represented in the file? signed / unsigned / float?
	1.5 Endianness - is each sample stored as little or big endian order.
2. read raw frames from the file, and give it to ALSA for playback.
	sndfile allows us to conveniently read frames in file, instead of calculating buffers offsets
	that should take into account the file header, number of channels, and sample byte size.

Then we open an alsa pcm device which is used to configure and transfer frames for playback by alsa.
*/



// forward declerations
class SndfileHandle;

namespace wavplayeralsa {

	class SingleFilePlayer {

	public:

		SingleFilePlayer();
		~SingleFilePlayer();

		const std::string &getFileToPlay();
		void initialize(const std::string &path, const std::string &fileName, StatusUpdateMsg *statusReporter);
		void startPlay(uint32_t positionInMs);
		void stop();

	private:
		void initSndFile();
		void initAlsa();
		bool isAlsaStatePlaying();
		void checkSongStartTime();

	private:
		void playLoopOnThread();


	private:

		std::string m_fileToPlay;
		std::string m_fullFileName;

	private:
	    enum SampleType {
	    	SampleTypeSigned = 0,
	    	SampleTypeUnsigned = 1,
	    	SampleTypeFloat = 2
	    };
	    unsigned int m_frameRate = 44100;
	    unsigned int m_channels = 2;
	    bool m_isEndianLittle = true; // if false the endian is big :)
	    SampleType m_sampleType = SampleTypeSigned;
	    unsigned int m_sampleChannelSizeBytes = 2;

	private:
		bool GetFormatForAlsa(snd_pcm_format_t &outFormat);
		snd_pcm_t *m_alsaPlaybackHandle;

	private:
		// snd file stuff
		SndfileHandle m_sndFile;
		unsigned int m_bytesPerFrame = 1;
		unsigned int m_framesCapacityInBuffer = 0; // how many frames can be stored in a buffer with size TRANSFER_BUFFER_SIZE
	    uint64_t m_totalFrames = 0;

	private:
		// alsa frames transfer stuff
		static const int AVAIL_MIN = 4096; // tells alsa to return from wait when buffer has this size of free space
		static const int TRANSFER_BUFFER_SIZE = 4096 * 16; // 64KB this is the buffer used to pass frames to alsa. this is the maximum number of frames to pass as one chunk

		// what is the next frame to be delivered to alsa
		uint64_t m_currPositionInFrames = 0;

	private:
		std::thread *m_playingThread = NULL;
		std::atomic_bool m_shouldBePlaying; // used to cancel the playing thread

	private:
		uint64_t m_songStartTimeMsSinceEphoc = 0;
		StatusUpdateMsg *m_statusReporter = NULL;


	};

}


#endif //__SINGLE_FILE_PLAYER_H__