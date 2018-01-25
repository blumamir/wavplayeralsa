#ifndef __SINGLE_FILE_PLAYER_H__
#define __SINGLE_FILE_PLAYER_H__

#include <string>
#include <thread>
#include <mutex>

#include <alsa/asoundlib.h>

// forward declerations
class SndfileHandle;
//struct snd_pcm_t;

namespace wavplayeralsa {

	class SingleFilePlayer {

	public:

		~SingleFilePlayer();

		void setFileToPlay(const std::string &fullFileName);
		void initialize();
		void startPlay(unsigned int positionInMs);
		unsigned int getPositionInMs();

	private:
		void initSndFile();
		void initAlsa();

	private:
		void playLoopOnThread();

	private:

		std::string m_fileToPlay;

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
	    snd_pcm_sframes_t m_totalFrames = 0;
	    char *m_rawDataBuffer = NULL;
	    bool m_sndFileInitialized = false;

	private:
		bool GetFormatForAlsa(snd_pcm_format_t &outFormat);
		snd_pcm_t *m_alsaPlaybackHandle;
		snd_pcm_sframes_t m_currPositionInFrames = 0;
		std::mutex m_currPositionMutex;
		bool m_alsaInitialized = false;

	private:
		std::thread *m_playingThread = NULL;


	};

}


#endif //__SINGLE_FILE_PLAYER_H__