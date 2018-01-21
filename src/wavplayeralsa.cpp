
#include <iostream>
#include <sndfile.h>
#include <alsa/asoundlib.h>

enum SampleType {
	SampleTypeSigned = 0,
	SampleTypeUnsigned = 1,
	SampleTypeFloat = 2
};
SampleType sampleType = SampleTypeSigned;
int sample_channel_size_bytes = 2;
unsigned int framerate = 44100;
bool is_endian_little = true; // if false the endian is big :)

snd_pcm_t *playback_handle;
short buf [4096 * 16];
int channels = 1;
char *buffer = NULL;
size_t buf_size = 0;
snd_pcm_sframes_t total_frames = 0;
snd_pcm_sframes_t frames_offset = 0;
int offset = 0;

int playback_callback(snd_pcm_sframes_t nframes) {

	int err;

	snd_pcm_sframes_t delay;
	if( (err = snd_pcm_delay(playback_handle, &delay)) < 0) {
		std::cerr << "cannot query current offset in buffer " << std::endl;
	}
	std::cout << "in playback_callback " << delay << std::endl;

	snd_pcm_sframes_t framesToWrite = nframes;
	if(total_frames - frames_offset < nframes) {
		framesToWrite = total_frames - frames_offset;
	}

	if( (err = snd_pcm_writei(playback_handle, buffer + offset, framesToWrite)) < 0) {
		std::cerr << "write failed (" << snd_strerror(err) << ")" << std::endl;
	}
	offset += (err * channels * sample_channel_size_bytes);
	frames_offset += err;


	return err;
}

bool setFormat(int sndFileFormat) {

	int majorType = sndFileFormat & SF_FORMAT_TYPEMASK;
	int minorType = sndFileFormat & SF_FORMAT_SUBMASK;
	std::cout << "wav format. major: 0x" << std::hex << majorType << " minor: 0x" << std::hex << minorType << std::endl;

	switch(minorType) {
		case SF_FORMAT_PCM_S8: 
			sample_channel_size_bytes = 1;
			sampleType = SampleTypeSigned;
			break;
		case SF_FORMAT_PCM_16: 
			sample_channel_size_bytes = 2;
			sampleType = SampleTypeSigned;
			break;
		case SF_FORMAT_PCM_24: 
			sample_channel_size_bytes = 3;
			sampleType = SampleTypeSigned;
			break;
		case SF_FORMAT_PCM_32: 
			sample_channel_size_bytes = 4;
			sampleType = SampleTypeSigned;
			break;
		case SF_FORMAT_FLOAT:
			sample_channel_size_bytes = 4;
			sampleType = SampleTypeFloat;
			break;
		case SF_FORMAT_DOUBLE:
			sample_channel_size_bytes = 8;
			sampleType = SampleTypeFloat;
			break;
		default:
			std::cout << "the minor format for the wav file contains unsupported value: " << minorType << std::endl;
			return false;
	}

	switch(majorType) {
		case SF_FORMAT_WAV:
			is_endian_little = true;
			break;
		case SF_FORMAT_AIFF:
			is_endian_little = false;
			break;
		default:
			std::cout << "the major format for the wav file contains unsupported value: " << majorType << std::endl;
			return false;		
	}

	std::cout << "The wav file format contains " << sample_channel_size_bytes << " bytes per channel and is " << 
		(is_endian_little ? "little" : "big") << " endian" << std::endl;

	return true;
}

bool GetFormatForAlsa(snd_pcm_format_t &outFormat) {
	switch(sampleType) {

		case SampleTypeSigned: {
			if(is_endian_little) {
				switch(sample_channel_size_bytes) {
					case 1: outFormat = SND_PCM_FORMAT_S8; return true;
					case 2: outFormat = SND_PCM_FORMAT_S16_LE; return true;
					case 3: outFormat = SND_PCM_FORMAT_S24_LE; return true;
					case 4: outFormat = SND_PCM_FORMAT_S32_LE; return true;
				}
			}
			else {
				switch(sample_channel_size_bytes) {
					case 1: outFormat = SND_PCM_FORMAT_S8; return true;
					case 2: outFormat = SND_PCM_FORMAT_S16_BE; return true;
					case 3: outFormat = SND_PCM_FORMAT_S24_BE; return true;
					case 4: outFormat = SND_PCM_FORMAT_S32_BE; return true;
				}
			}
		}
		break;

		case SampleTypeUnsigned: {
			if(is_endian_little) {
				switch(sample_channel_size_bytes) {
					case 1: outFormat = SND_PCM_FORMAT_U8; return true;
					case 2: outFormat = SND_PCM_FORMAT_U16_LE; return true;
					case 3: outFormat = SND_PCM_FORMAT_U24_LE; return true;
					case 4: outFormat = SND_PCM_FORMAT_U32_LE; return true;
				}
			}
			else {
				switch(sample_channel_size_bytes) {
					case 1: outFormat = SND_PCM_FORMAT_U8; return true;
					case 2: outFormat = SND_PCM_FORMAT_U16_BE; return true;
					case 3: outFormat = SND_PCM_FORMAT_U24_BE; return true;
					case 4: outFormat = SND_PCM_FORMAT_U32_BE; return true;
				}
			}
		}
		break;

		case SampleTypeFloat: {
			if(is_endian_little) {
				switch(sample_channel_size_bytes) {
					case 4: outFormat = SND_PCM_FORMAT_FLOAT_LE; return true;
					case 8: outFormat = SND_PCM_FORMAT_FLOAT64_LE; return true;
				}
			}
			else {
				switch(sample_channel_size_bytes) {
					case 4: outFormat = SND_PCM_FORMAT_FLOAT_BE; return true;
					case 8: outFormat = SND_PCM_FORMAT_FLOAT64_BE; return true;
				}
			}			

		}
		break;

	}
	return false;
}


int main(int argc, char *argv[]) {

	if (argc <= 1) {
		std::cout << "Usage: " << argv[0] << " file.wav" << std::endl;
		return -1;
	}


	const char *filename = argv[1];

	// sndfile stuff
	SF_INFO sfinfo;
	SNDFILE *infile = NULL;
	infile = sf_open(filename, SFM_READ, &sfinfo);
	if(infile == NULL) {
		std::cerr << "the input wav file cannot be opened" << std::endl;
		return -1;
	}
	std::cout << "Channels: " << sfinfo.channels << std::endl;
	std::cout << "Sample rate: " << sfinfo.samplerate << std::endl;
	std::cout << "Sections: " << sfinfo.sections << std::endl;
	std::cout << "Format: " << sfinfo.format << std::endl;

	channels = sfinfo.channels;
	framerate = sfinfo.samplerate;
	if(setFormat(sfinfo.format) == false) {
		std::cerr << "the format of the current wav file is not supported" << std::endl;
	}
	buf_size = sfinfo.frames * channels * sample_channel_size_bytes;
	total_frames = sfinfo.frames;
    std::cout << "buffer size: " << buf_size << std::endl;
    buffer = new char[buf_size];
    sf_read_raw(infile, buffer, buf_size);

	// alsa stuff
	int err;

	const char *audioDevice = "plughw:0,0";
	if( (err = snd_pcm_open(&playback_handle, audioDevice, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		std::cerr << "cannot open audio device " << audioDevice << " (" << snd_strerror(err) << ")" << std::endl;
		return -1;
	}


	// set hw parameters

	snd_pcm_hw_params_t *hw_params;

	if( (err = snd_pcm_hw_params_malloc(&hw_params)) < 0 ) {
		std::cerr << "cannot allocate hardware parameter structure (" << snd_strerror(err) << ")" << std::endl;
		return -1;		
	}

	if( (err = snd_pcm_hw_params_any(playback_handle, hw_params)) < 0) {
		std::cerr << "cannot initialize hardware parameter structure (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	if( (err = snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		std::cerr << "cannot set access type (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	snd_pcm_format_t alsaFormat;
	if(GetFormatForAlsa(alsaFormat) != true) {
		std::cerr << "the wav format is not supported by this player of alsa" << std::endl;
		return -1;						
	}
	if( (err = snd_pcm_hw_params_set_format(playback_handle, hw_params, alsaFormat)) < 0) {
		std::cerr << "cannot set sample format (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	if( (err = snd_pcm_hw_params_set_rate(playback_handle, hw_params, framerate, 0)) < 0) {
		std::cerr << "cannot set sample rate (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	if( (err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, channels)) < 0) {
		std::cerr << "cannot set channel count (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	if( (err = snd_pcm_hw_params(playback_handle, hw_params)) < 0) {
		std::cerr << "cannot set parameters (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	snd_pcm_hw_params_free(hw_params);
	hw_params = NULL;

	// set software parameters

	snd_pcm_sw_params_t *sw_params;

	if( (err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
		std::cerr << "cannot allocate software parameters structure (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	if( (err = snd_pcm_sw_params_current(playback_handle, sw_params)) < 0) {
		std::cerr << "cannot initialize software parameters structure (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	if( (err = snd_pcm_sw_params_set_avail_min(playback_handle, sw_params, 4096)) < 0) {
		std::cerr << "cannot set minimum available count (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}	

	if( (err = snd_pcm_sw_params_set_start_threshold(playback_handle, sw_params, 0U)) < 0) {
		std::cerr << "cannot set start mode (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}	

	if( (err = snd_pcm_sw_params(playback_handle, sw_params)) < 0) {
		std::cerr << "cannot set software parameters (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}	

	snd_pcm_sw_params_free(sw_params);
	sw_params = NULL;

	// main loop
	if( (err = snd_pcm_prepare(playback_handle)) < 0) {
		std::cerr << "cannot prepare audio interface for use (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}	

	while(1) {

		if( (err = snd_pcm_wait(playback_handle, 1000)) < 0) {
			std::cerr << "pool failed (" << snd_strerror(err) << ")" << std::endl;
			return -1;							
		}

		snd_pcm_sframes_t frames_to_deliver;
		if( (frames_to_deliver = snd_pcm_avail_update(playback_handle)) < 0) {
			if(frames_to_deliver == -EPIPE) {
				std::cerr << "an xrun occured" << std::endl;
				break;
			}
			else {
				std::cerr << "unknown ALSA avail update return value (" << frames_to_deliver << ")" << std::endl;
				break;
			}
		}

		frames_to_deliver = frames_to_deliver > 4096 ? 4096 : frames_to_deliver;

		if(playback_callback(frames_to_deliver) != frames_to_deliver) {
			std::cout << "ended" << std::endl;
			break;
		}

	}

	while(snd_pcm_avail(playback_handle) > 0) {
		snd_pcm_sframes_t delay;
		if( (err = snd_pcm_delay(playback_handle, &delay)) < 0) {
			std::cerr << "cannot query current offset in buffer " << std::endl;
		}		
		std::cout << "delay = " << delay << std::endl;
	}

	snd_pcm_close(playback_handle);
	delete buffer;

	std::cout << "done" << std::endl;

	return 0;
}

