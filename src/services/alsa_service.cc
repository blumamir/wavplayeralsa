#include "services/alsa_service.h"

#include <sstream>
#include <iostream>

#include "alsa/asoundlib.h"
#include "sndfile.hh"
#include "spdlog/spdlog.h"
#include "spdlog/async.h"

namespace wavplayeralsa
{

    class AlsaPlaybackService : public IAlsaPlaybackService
    {

    public:

        AlsaPlaybackService(
            std::shared_ptr<spdlog::logger> logger,
            const std::string &full_file_name, 
            const std::string &file_id,
			const std::string &audio_device
        );

		~AlsaPlaybackService();

    private:

        void InitSndFile(const std::string &full_file_name);  
		void InitAlsa(const std::string &audio_device);      


    private:
        std::shared_ptr<spdlog::logger> logger_;

    // alsa
    private:
    	static const int TRANSFER_BUFFER_SIZE = 4096 * 16; // 64KB this is the buffer used to pass frames to alsa. this is the maximum number of bytes to pass as one chunk

    // snd file
    private:
    	SndfileHandle snd_file_;

	    enum SampleType {
	    	SampleTypeSigned = 0,
	    	SampleTypeUnsigned = 1,
	    	SampleTypeFloat = 2
	    };
	    static const char *SampleTypeToString(SampleType sample_type);
		bool GetFormatForAlsa(snd_pcm_format_t &out_format) const;

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
		
		// alsa
		snd_pcm_t *alsa_playback_handle_ = nullptr;


    };

    AlsaPlaybackService::AlsaPlaybackService(
            std::shared_ptr<spdlog::logger> logger,
            const std::string &full_file_name, 
            const std::string &file_id,
			const std::string &audio_device
        ) :
            logger_(logger)
    {
        InitSndFile(full_file_name);
		InitAlsa(audio_device);
    }

	AlsaPlaybackService::~AlsaPlaybackService() {

		if(alsa_playback_handle_ != nullptr) {
			snd_pcm_close(alsa_playback_handle_);
			alsa_playback_handle_ = nullptr;
		}

	}

	/*
	Read the file content from disk, extract relevant metadata from the
	wav header, and save it to the relevant members of the class.
	The function will also initialize the snd_file member, which allows to
	read the wav file frames.
	Will throw std::runtime_error in case of error.
	 */
    void AlsaPlaybackService::InitSndFile(const std::string &full_file_name)
    {
		snd_file_ = SndfileHandle(full_file_name);
		if(snd_file_.error() != 0) {
			std::stringstream errorDesc;
			errorDesc << "The file '" << full_file_name << "' cannot be opened. error msg: '" << snd_file_.strError() << "'";
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
				full_file_name, frame_rate_, num_of_channels_, major_type, minor_type, bytes_per_sample_, 
				SampleTypeToString(sample_type_), 
				(is_endian_little_ ? "little" : "big"),
				total_frame_in_file_, number_of_ms, number_of_minutes, seconds_modulo
			);

    }

	const char *AlsaPlaybackService::SampleTypeToString(SampleType sample_type) {
		switch(sample_type) {
	    	case SampleTypeSigned: return "signed integer";
	    	case SampleTypeUnsigned: return "unsigned integer";
	    	case SampleTypeFloat: return "float";
		}
		std::stringstream err_desc;
		err_desc << "sample type not supported. value is " << (int)sample_type;
		throw std::runtime_error(err_desc.str());
	}

	/*
	Init the alsa driver according to the params of the current wav file.
	throw std::runtime_error in case of error
	 */
	void AlsaPlaybackService::InitAlsa(const std::string &audio_device) {

		int err;
		std::stringstream err_desc;

		if( (err = snd_pcm_open(&alsa_playback_handle_, audio_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
			err_desc << "cannot open audio device " << audio_device << " (" << snd_strerror(err) << ")";
			throw std::runtime_error(err_desc.str());
		}

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

	bool AlsaPlaybackService::GetFormatForAlsa(snd_pcm_format_t &out_format) const {
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

    void AlsaPlaybackServiceFactory::Initialize(
            std::shared_ptr<spdlog::logger> logger,
            const std::string &audio_device
        )
    {
        logger_ = logger;
        audio_device_ = audio_device;
    }

    IAlsaPlaybackService* AlsaPlaybackServiceFactory::CreateAlsaPlaybackService(
            const std::string &full_file_name, 
            const std::string &file_id
        )
    {
        return new AlsaPlaybackService(
            logger_->clone("alsa_playback_service"), // TODO - use file name or id
            full_file_name,
            file_id,
			audio_device_
        );
    }

}