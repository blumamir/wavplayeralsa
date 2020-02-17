#ifndef WAVPLAYERALSA_ALSA_FRAMES_TRANSFER_H_
#define WAVPLAYERALSA_ALSA_FRAMES_TRANSFER_H_

#include <cstdint>
#include <atomic>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "alsa/asoundlib.h"
#include "sndfile.hh"
#include "spdlog/spdlog.h"

#include "player_events_ifc.h"

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

namespace wavplayeralsa {

	class AlsaFramesTransfer {

	public:

		AlsaFramesTransfer();
		~AlsaFramesTransfer();

		// can throw exception
		void Initialize(std::shared_ptr<spdlog::logger> logger, 
			PlayerEventsIfc *player_events_callback, 
			const std::string &audio_device);

		// file id is a string identifier that is used to refer to the file being played.
		// it is something like: beep.wav or beatles/let_it_be.wav
		// what should *NOT* be used as fileId is: ../../songs/beep.wav songs//beep.wav ./beep.wav
		// it should be canonical, so that each file is always identify uniquely
		const std::string &GetFileId() const;
		bool LoadNewFile(const std::string &full_file_name, const std::string &file_id);
		void StartPlay(uint32_t position_in_ms, uint32_t play_seq_id);
		bool Stop();

	private:
		void InitSndFile();
		void InitAlsa();
		bool GetFormatForAlsa(snd_pcm_format_t &out_format) const;

	private:
		void TransferFramesWrapper(uint32_t play_seq_id);
		bool IsAlsaStatePlaying();
		void CheckSongStartTime(uint32_t play_seq_id);
		// once all frames are written to pcm, this function runs until pcm is empty
		void PcmDrainLoop(boost::system::error_code error_code, uint32_t play_seq_id);
		void FramesToPcmTransferLoop(boost::system::error_code error_code, uint32_t play_seq_id);
		void PcmDrop();

	private:
		std::shared_ptr<spdlog::logger> logger_;

	private:
		// initialization detection
		bool initialized_ = false;
		bool snd_initialized_ = false; // if true -> sound file is loaded successfully, and alsa is configured to support it

	private:
		// current loaded file
		std::string file_id_;
		std::string full_file_name_;

	private:
		// snd file stuff

	    enum SampleType {
	    	SampleTypeSigned = 0,
	    	SampleTypeUnsigned = 1,
	    	SampleTypeFloat = 2
	    };
	    static const char *SampleTypeToString(SampleType sample_type);

		SndfileHandle snd_file_;

		// from file
	    unsigned int frame_rate_ = 44100;
	    unsigned int num_of_channels_ = 2;
	    bool is_endian_little_ = true; // if false the endian is big :)
	    SampleType sample_type_ = SampleTypeSigned;
	    unsigned int bytes_per_sample_ = 2;
	    uint64_t total_frame_in_file_ = 0;

	    // calculated
		unsigned int bytes_per_frame_ = 1;
		snd_pcm_sframes_t frames_capacity_in_buffer_ = 0; // how many frames can be stored in a buffer with size TRANSFER_BUFFER_SIZE

	private:
		// alsa frames transfer stuff
		snd_pcm_t *alsa_playback_handle_;

		static const int TRANSFER_BUFFER_SIZE = 4096 * 16; // 64KB this is the buffer used to pass frames to alsa. this is the maximum number of bytes to pass as one chunk

		// what is the next frame to be delivered to alsa
		uint64_t curr_position_frames_ = 0;

	private:
		// transfer thread
		boost::asio::io_service alsa_ios_;
		boost::asio::deadline_timer alsa_wait_timer_;
		std::thread playing_thread_;

	private:
		// position reporting
		uint64_t audio_start_time_ms_since_epoch_ = 0;
		PlayerEventsIfc *player_events_callback_ = nullptr;


	};

}


#endif // WAVPLAYERALSA_ALSA_FRAMES_TRANSFER_H_