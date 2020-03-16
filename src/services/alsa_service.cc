#include "services/alsa_service.h"

#include <sstream>

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
            const std::string &file_id
        );

    private:

        void InitSndFile(const std::string &full_file_name);        


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

    };

    AlsaPlaybackService::AlsaPlaybackService(
            std::shared_ptr<spdlog::logger> logger,
            const std::string &full_file_name, 
            const std::string &file_id
        ) :
            logger_(logger)
    {
        InitSndFile(full_file_name);
    }

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
            logger_->clone("alsa_playback_serrvice"), // TODO - use file name or id
            full_file_name,
            file_id
        );
    }

}