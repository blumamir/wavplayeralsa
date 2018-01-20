
#include <iostream>
#include <sndfile.h>
#include <alsa/asoundlib.h>

snd_pcm_t *playback_handle;
short buf [4096 * 16];
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
	offset += (err * 2 * 2);
	frames_offset += err;


	return err;
}

int main(int argc, char *argv[]) {

	const char *filename = "/home/amir/Repos/wavplayer/sample_7sec.wav";

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

	buf_size = sfinfo.frames * sfinfo.channels * 2;
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

	if( (err = snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		std::cerr << "cannot set sample format (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	unsigned int sample_rate = 44100;
	if( (err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, &sample_rate, 0)) < 0) {
		std::cerr << "cannot set sample rate (" << snd_strerror(err) << ")" << std::endl;
		return -1;				
	}

	if( (err = snd_pcm_hw_params_set_channels(playback_handle, hw_params, 2)) < 0) {
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

