#include "alsa_frames_transfer.h"

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <sys/time.h>

namespace wavplayeralsa {

	AlsaFramesTransfer::AlsaFramesTransfer() : 
		alsa_wait_timer_(alsa_ios_)
	{
		// we use the stop flag to indicate if a song is currently playing or not
		alsa_ios_.stop();
	}
	

	AlsaFramesTransfer::~AlsaFramesTransfer() {

		Stop();

		if(alsa_playback_handle_ != nullptr) {
			snd_pcm_close(alsa_playback_handle_);
			alsa_playback_handle_ = nullptr;
		}
	}

	const std::string &AlsaFramesTransfer::GetFileId() const { 
		return file_id_; 
	}

	void AlsaFramesTransfer::Initialize(std::shared_ptr<spdlog::logger> logger, 
			PlayerEventsIfc *player_events_callback, 
			const std::string &audio_device) 
	{

		if(initialized_) {
			throw std::runtime_error("Initialize called on an already initialized alsa player");
		}

		player_events_callback_ = player_events_callback;
		logger_ = logger;

		int err;
		std::stringstream err_desc;

		// audio_device should be somthing like "plughw:0,0", "default"
		if( (err = snd_pcm_open(&alsa_playback_handle_, audio_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
			err_desc << "cannot open audio device " << audio_device << " (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		initialized_ = true;
	}

	void AlsaFramesTransfer::CheckSongStartTime(uint32_t play_seq_id) {
		int err;
		snd_pcm_sframes_t delay = 0;
		int64_t pos_in_frames = 0;

		if( (err = snd_pcm_delay(alsa_playback_handle_, &delay)) < 0) {
			std::stringstream err_desc;
			err_desc << "cannot query current offset in buffer (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}	 

		// this is a magic number test to remove end of file wrong reporting
		if(delay < 4096) {
			return;
		}
		pos_in_frames = curr_position_frames_ - delay; 			
		int64_t ms_since_audio_file_start = ((pos_in_frames * (int64_t)1000) / (int64_t)frame_rate_);

		struct timeval tv;
		gettimeofday(&tv, NULL);
		// convert sec to ms and usec to ms
		uint64_t curr_time_ms_since_epoch = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
		uint64_t audio_file_start_time_ms_since_epoch = (int64_t)curr_time_ms_since_epoch - ms_since_audio_file_start;

		int64_t diff_from_prev = audio_file_start_time_ms_since_epoch - audio_start_time_ms_since_epoch_;
		// there might be small jittering, we don't want to update the value often.
		if(diff_from_prev > 1 || diff_from_prev < -1) {
			player_events_callback_->NewSongStatus(file_id_, play_seq_id, audio_file_start_time_ms_since_epoch, 1.0);

			std::stringstream msg_stream;
			msg_stream << "calculated a new audio file start time: " << audio_file_start_time_ms_since_epoch << " (ms since epoch). ";
			if(audio_start_time_ms_since_epoch_ > 0) {
				msg_stream << "this is a change since last calculation of " << diff_from_prev << " ms. ";
			}
			msg_stream << "pcm delay in frames as reported by alsa: " << delay << " and position in file is " << 
				ms_since_audio_file_start << " ms. ";
			logger_->info(msg_stream.str());
		}
		audio_start_time_ms_since_epoch_ = audio_file_start_time_ms_since_epoch;

	}

	void AlsaFramesTransfer::TransferFramesWrapper(uint32_t play_seq_id) {

		try {
			alsa_ios_.reset();
			alsa_ios_.post(std::bind(&AlsaFramesTransfer::FramesToPcmTransferLoop, this, boost::system::error_code(), play_seq_id));
			alsa_ios_.run();
			PcmDrop();
		}
		catch(const std::runtime_error &e) {
			logger_->error("error while playing current wav file. stopped transfering frames to alsa. exception is: {}", e.what());
		}
		player_events_callback_->NoSongPlayingStatus(file_id_, play_seq_id);
	}

	void AlsaFramesTransfer::FramesToPcmTransferLoop(boost::system::error_code error_code, uint32_t play_seq_id) {

		// the function might be called from timer, in which case error_code might
		// indicate the timer canceled and we should not invoke the function.
		if(error_code)
			return;

		std::stringstream err_desc;
		int err;

		// calculate how many frames to write
		snd_pcm_sframes_t frames_to_deliver;
		if( (frames_to_deliver = snd_pcm_avail_update(alsa_playback_handle_)) < 0) {
			if(frames_to_deliver == -EPIPE) {
				throw std::runtime_error("an xrun occured");
			}
			else {
				err_desc << "unknown ALSA avail update return value (" << frames_to_deliver << ")";
				throw std::runtime_error(err_desc.str());
			}
		}
		else if(frames_to_deliver == 0) {
			alsa_wait_timer_.expires_from_now(boost::posix_time::millisec(5));
			alsa_wait_timer_.async_wait(std::bind(&AlsaFramesTransfer::FramesToPcmTransferLoop, this, std::placeholders::_1, play_seq_id));
			return;
		}

		// we want to deliver as many frames as possible.
		// we can put frames_to_deliver number of frames, but the buffer can only hold frames_capacity_in_buffer_ frames
		frames_to_deliver = std::min(frames_to_deliver, frames_capacity_in_buffer_);
		unsigned int bytes_to_deliver = frames_to_deliver * bytes_per_frame_;

		// read the frames from the file. TODO: what if readRaw fails?
		char buffer_for_transfer[TRANSFER_BUFFER_SIZE];
		bytes_to_deliver = snd_file_.readRaw(buffer_for_transfer, bytes_to_deliver);
		if(bytes_to_deliver < 0) {
			err_desc << "Failed reading raw frames from snd file. returned: " << sf_error_number(bytes_to_deliver);
			throw std::runtime_error(err_desc.str());				
		}
		if(bytes_to_deliver == 0) {
			logger_->info("done writing all frames to pcm. waiting for audio device to play remaining frames in the buffer");
			alsa_ios_.post(std::bind(&AlsaFramesTransfer::PcmDrainLoop, this, boost::system::error_code(), play_seq_id));
			return;
		}

		int frames_written = snd_pcm_writei(alsa_playback_handle_, buffer_for_transfer, frames_to_deliver);
		if( frames_written < 0) {
			err_desc << "snd_pcm_writei failed (" << snd_strerror(frames_written) << ")";
			throw std::runtime_error(err_desc.str());				
		}

		curr_position_frames_ += frames_written;
		if(frames_written != frames_to_deliver) {
			logger_->warn("transfered to alsa less frame then requested. frames_to_deliver: {}, frames_written: {}", frames_to_deliver, frames_written);
			snd_file_.seek(curr_position_frames_, SEEK_SET);
		}

		CheckSongStartTime(play_seq_id);

		alsa_ios_.post(std::bind(&AlsaFramesTransfer::FramesToPcmTransferLoop, this, boost::system::error_code(), play_seq_id));
	}

	void AlsaFramesTransfer::PcmDrainLoop(boost::system::error_code error_code, uint32_t play_seq_id) {

		if(error_code)
			return;

		bool is_currently_playing = IsAlsaStatePlaying();

		if(!is_currently_playing) {
			logger_->info("playing audio file ended successfully (transfered all frames to pcm and it is empty)");
			return;
		}

		CheckSongStartTime(play_seq_id);

		alsa_wait_timer_.expires_from_now(boost::posix_time::millisec(5));
		alsa_wait_timer_.async_wait(std::bind(&AlsaFramesTransfer::PcmDrainLoop, this, std::placeholders::_1, play_seq_id));
	}

	void AlsaFramesTransfer::PcmDrop() 
	{
		int err;
		if( (err = snd_pcm_drop(alsa_playback_handle_)) < 0 ) {
			std::stringstream err_desc;
			err_desc << "snd_pcm_drop failed (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}
	}

	bool AlsaFramesTransfer::IsAlsaStatePlaying() 
	{
		int status = snd_pcm_state(alsa_playback_handle_);
		return (status == SND_PCM_STATE_RUNNING) || (status == SND_PCM_STATE_PREPARED);
	}

	void AlsaFramesTransfer::StartPlay(uint32_t position_in_ms, uint32_t play_seq_id) 
	{
		if(!snd_initialized_) {
			throw std::runtime_error("the player is not initialized with a valid sound file.");
		}

		Stop();

		double position_in_seconds = (double)position_in_ms / 1000.0;
		curr_position_frames_ = position_in_seconds * (double)frame_rate_; 
		if(curr_position_frames_ > total_frame_in_file_) {
			curr_position_frames_ = total_frame_in_file_;
		}
		sf_count_t seek_res = snd_file_.seek(curr_position_frames_, SEEK_SET);

		int err;
		std::stringstream err_desc;
		if( (err = snd_pcm_prepare(alsa_playback_handle_)) < 0 ) {
			err_desc << "snd_pcm_prepare failed (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		logger_->info("start playing file {} from position {} mili-seconds ({} seconds)", file_id_, position_in_ms, position_in_seconds);
		playing_thread_ = std::thread(&AlsaFramesTransfer::TransferFramesWrapper, this, play_seq_id);
	}

	/*
	stop the current audio file from being played.
	the following scenarios are possible:
	1. alsa's pcm is running, transfering frames to the audio device, and the monitoring thread is active
		and serving frames \ waiting for buffer to drain.
	2. alsa pcm is drained, not more frames are sent to audio device, and the monitoring thread has finished
		execution and waiting to be joined.
	3. the thread has been joined, thus there is noting to stop.

	The function returns true if it's invocation is what made the audio device stop playing (case 1).
	It will return false if stop is called but the pcm is not running anyway (cases 2, 3)
	*/
	bool AlsaFramesTransfer::Stop() {

		bool is_playing = !(alsa_ios_.stopped());
		
		alsa_wait_timer_.cancel();
		alsa_ios_.stop();

		if(playing_thread_.joinable()) {
			playing_thread_.join();
		}
		audio_start_time_ms_since_epoch_ = 0; // invalidate old start time so on next play a new status will be sent
		return is_playing; // if we were playing when we entered the function, then the invocation is what made it stop
	}

	/*
	return true if loading a new file caused the current audio to 'stop', return false if no file was playing anyway.
	*/
	bool AlsaFramesTransfer::LoadNewFile(const std::string &full_file_name, const std::string &file_id) {

		if(!initialized_) {
			throw std::runtime_error("LoadNewFile called but player not initilized");
		}

		bool was_playing = this->Stop();

		// mark snd as not initialized. after everything goes well, and no exception is thrown, it will be changed to initialized
		snd_initialized_ = false;

		full_file_name_ = full_file_name;
		file_id_ = file_id;
		InitSndFile();	
		InitAlsa();	

		snd_initialized_ = true;

		return was_playing;
	}

	void AlsaFramesTransfer::InitSndFile() {

		snd_file_ = SndfileHandle(full_file_name_);
		if(snd_file_.error() != 0) {
			std::stringstream errorDesc;
			errorDesc << "The file '" << full_file_name_ << "' cannot be opened. error msg: '" << snd_file_.strError() << "'";
			throw std::runtime_error(errorDesc.str());
		}

		// set the parameters from read from the SndFile and produce log messages

		frame_rate_ = snd_file_.samplerate();
		num_of_channels_ = snd_file_.channels();

		int major_type = snd_file_.format() & SF_FORMAT_TYPEMASK;
		int minor_type = snd_file_.format() & SF_FORMAT_SUBMASK;

		switch(minor_type) {
			case SF_FORMAT_PCM_S8: 
				bytes_per_sample_ = 1;
				sample_type_ = SampleTypeSigned;
				break;
			case SF_FORMAT_PCM_16: 
				bytes_per_sample_ = 2;
				sample_type_ = SampleTypeSigned;
				break;
			case SF_FORMAT_PCM_24: 
				bytes_per_sample_ = 3;
				sample_type_ = SampleTypeSigned;
				break;
			case SF_FORMAT_PCM_32: 
				bytes_per_sample_ = 4;
				sample_type_ = SampleTypeSigned;
				break;
			case SF_FORMAT_FLOAT:
				bytes_per_sample_ = 4;
				sample_type_ = SampleTypeFloat;
				break;
			case SF_FORMAT_DOUBLE:
				bytes_per_sample_ = 8;
				sample_type_ = SampleTypeFloat;
				break;
			default:
				std::stringstream err_desc;
				err_desc << "wav file is in unsupported format. minor format as read from sndFile is: " << std::hex << minor_type;
				throw std::runtime_error(err_desc.str());
		}

		switch(major_type) {
			case SF_FORMAT_WAV:
				is_endian_little_ = true;
				break;
			case SF_FORMAT_AIFF:
				is_endian_little_ = false;
				break;
			default:
				std::stringstream err_desc;
				err_desc << "wav file is in unsupported format. major format as read from sndFile is: " << std::hex << major_type;
				throw std::runtime_error(err_desc.str());
		}

		total_frame_in_file_ = snd_file_.frames();
		uint64_t number_of_ms = total_frame_in_file_ * 1000 / frame_rate_;
		int number_of_minutes = number_of_ms / (1000 * 60);
		int seconds_modulo = (number_of_ms / 1000) % 60;	

		bytes_per_frame_ = num_of_channels_ * bytes_per_sample_;
		frames_capacity_in_buffer_ = (snd_pcm_sframes_t)(TRANSFER_BUFFER_SIZE / bytes_per_frame_);

		logger_->info("finished reading audio file '{}'. "
			"Frame rate: {} frames per seconds, "
			"Number of channels: {}, "
			"Wav format: major 0x{:x}, minor 0x{:x}, "
			"Bytes per sample: {}, "
			"Sample type: '{}', "
			"Endian: '{}', "
			"Total frames in file: {} which are: {} ms, and {}:{} minutes", 
				full_file_name_, frame_rate_, num_of_channels_, major_type, minor_type, bytes_per_sample_, 
				SampleTypeToString(sample_type_), 
				(is_endian_little_ ? "little" : "big"),
				total_frame_in_file_, number_of_ms, number_of_minutes, seconds_modulo
			);
	}

	bool AlsaFramesTransfer::GetFormatForAlsa(snd_pcm_format_t &out_format) const {
		switch(sample_type_) {

			case SampleTypeSigned: {
				if(is_endian_little_) {
					switch(bytes_per_sample_) {
						case 1: out_format = SND_PCM_FORMAT_S8; return true;
						case 2: out_format = SND_PCM_FORMAT_S16_LE; return true;
						case 3: out_format = SND_PCM_FORMAT_S24_LE; return true;
						case 4: out_format = SND_PCM_FORMAT_S32_LE; return true;
					}
				}
				else {
					switch(bytes_per_sample_) {
						case 1: out_format = SND_PCM_FORMAT_S8; return true;
						case 2: out_format = SND_PCM_FORMAT_S16_BE; return true;
						case 3: out_format = SND_PCM_FORMAT_S24_BE; return true;
						case 4: out_format = SND_PCM_FORMAT_S32_BE; return true;
					}
				}
			}
			break;

			case SampleTypeUnsigned: {
				if(is_endian_little_) {
					switch(bytes_per_sample_) {
						case 1: out_format = SND_PCM_FORMAT_U8; return true;
						case 2: out_format = SND_PCM_FORMAT_U16_LE; return true;
						case 3: out_format = SND_PCM_FORMAT_U24_LE; return true;
						case 4: out_format = SND_PCM_FORMAT_U32_LE; return true;
					}
				}
				else {
					switch(bytes_per_sample_) {
						case 1: out_format = SND_PCM_FORMAT_U8; return true;
						case 2: out_format = SND_PCM_FORMAT_U16_BE; return true;
						case 3: out_format = SND_PCM_FORMAT_U24_BE; return true;
						case 4: out_format = SND_PCM_FORMAT_U32_BE; return true;
					}
				}
			}
			break;

			case SampleTypeFloat: {
				if(is_endian_little_) {
					switch(bytes_per_sample_) {
						case 4: out_format = SND_PCM_FORMAT_FLOAT_LE; return true;
						case 8: out_format = SND_PCM_FORMAT_FLOAT64_LE; return true;
					}
				}
				else {
					switch(bytes_per_sample_) {
						case 4: out_format = SND_PCM_FORMAT_FLOAT_BE; return true;
						case 8: out_format = SND_PCM_FORMAT_FLOAT64_BE; return true;
					}
				}			

			}
			break;

		}
		return false;
	}


	void AlsaFramesTransfer::InitAlsa() {

		int err;
		std::stringstream err_desc;

		// set hw parameters

		snd_pcm_hw_params_t *hw_params;

		if( (err = snd_pcm_hw_params_malloc(&hw_params)) < 0 ) {
			err_desc << "cannot allocate hardware parameter structure (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		if( (err = snd_pcm_hw_params_any(alsa_playback_handle_, hw_params)) < 0) {
			err_desc << "cannot initialize hardware parameter structure (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		if( (err = snd_pcm_hw_params_set_access(alsa_playback_handle_, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			err_desc << "cannot set access type (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		snd_pcm_format_t alsaFormat;
		if(GetFormatForAlsa(alsaFormat) != true) {
			err_desc << "the wav format is not supported by this player of alsa";
			throw std::runtime_error(err_desc.str());
		}
		if( (err = snd_pcm_hw_params_set_format(alsa_playback_handle_, hw_params, alsaFormat)) < 0) {
			err_desc << "cannot set sample format (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		if( (err = snd_pcm_hw_params_set_rate(alsa_playback_handle_, hw_params, frame_rate_, 0)) < 0) {
			err_desc << "cannot set sample rate (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		if( (err = snd_pcm_hw_params_set_channels(alsa_playback_handle_, hw_params, num_of_channels_)) < 0) {
			err_desc << "cannot set channel count (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		if( (err = snd_pcm_hw_params(alsa_playback_handle_, hw_params)) < 0) {
			err_desc << "cannot set alsa hw parameters (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		snd_pcm_hw_params_free(hw_params);
		hw_params = nullptr;


		// set software parameters

		snd_pcm_sw_params_t *sw_params;

		if( (err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
			err_desc << "cannot allocate software parameters structure (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		if( (err = snd_pcm_sw_params_current(alsa_playback_handle_, sw_params)) < 0) {
			err_desc << "cannot initialize software parameters structure (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

		// we transfer frames to alsa buffers, and then call snd_pcm_wait and block until buffer
		// has more space for next frames.
		// this parameter is the amount of min availible frames in the buffer that should trigger
		// alsa to notify us for writing more frames.
		if( (err = snd_pcm_sw_params_set_avail_min(alsa_playback_handle_, sw_params, AVAIL_MIN)) < 0) {
			err_desc << "cannot set minimum available count (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}	

		// how many frames should be in the buffer before alsa start to play it.
		// we set to 0 -> means start playing immediately
		if( (err = snd_pcm_sw_params_set_start_threshold(alsa_playback_handle_, sw_params, 0U)) < 0) {
			err_desc << "cannot set start mode (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}	

		if( (err = snd_pcm_sw_params(alsa_playback_handle_, sw_params)) < 0) {
			err_desc << "cannot set software parameters (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}	

		snd_pcm_sw_params_free(sw_params);
		sw_params = nullptr;

		if( (err = snd_pcm_prepare(alsa_playback_handle_)) < 0) {
			err_desc << "cannot prepare audio interface for use (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}	

	}

	const char *AlsaFramesTransfer::SampleTypeToString(SampleType sample_type) {
		switch(sample_type) {
	    	case SampleTypeSigned: return "signed integer";
	    	case SampleTypeUnsigned: return "unsigned integer";
	    	case SampleTypeFloat: return "float";
		}
		std::stringstream err_desc;
		err_desc << "sample type not supported. value is " << (int)sample_type;
		throw std::runtime_error(err_desc.str());
	}


}







