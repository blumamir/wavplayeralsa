#include "single_file_player.h"

#include <sndfile.hh>
#include <alsa/asoundlib.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>

namespace wavplayeralsa {

	SingleFilePlayer::~SingleFilePlayer() {

		if(m_rawDataBuffer != NULL) {
			delete m_rawDataBuffer;
			m_rawDataBuffer = NULL;
		}

		if(m_alsaPlaybackHandle != NULL) {
			snd_pcm_close(m_alsaPlaybackHandle);
			m_alsaPlaybackHandle = NULL;
		}
	}

	const std::string &SingleFilePlayer::getFileToPlay() { 
		return m_fileToPlay; 
	}

	void SingleFilePlayer::playLoopOnThread() {

		int err;

		while(1) {

			m_playStatusMutex.lock();
			if(m_playStatus != Playing) {

				// clear the content of the buffer
				int err;
				std::stringstream errDesc;
				if( (err = snd_pcm_drop(m_alsaPlaybackHandle)) < 0 ) {
					errDesc << "snd_pcm_drop failed (" << snd_strerror(err) << ")";
					m_playStatusMutex.unlock();
					throw std::runtime_error(errDesc.str());
				}
				m_playStatus = Stopped;
				m_playStatusMutex.unlock();
				return;
			}
			m_playStatusMutex.unlock();

			if( (err = snd_pcm_wait(m_alsaPlaybackHandle, 1000)) < 0) {
				std::cerr << "pool failed (" << snd_strerror(err) << ")" << std::endl;
				break;
			}

			// calculate how many frames to write
			snd_pcm_sframes_t framesToDeliver;
			if( (framesToDeliver = snd_pcm_avail_update(m_alsaPlaybackHandle)) < 0) {
				if(framesToDeliver == -EPIPE) {
					std::cerr << "an xrun occured" << std::endl;
					break;
				}
				else {
					std::cerr << "unknown ALSA avail update return value (" << framesToDeliver << ")" << std::endl;
					break;
				}
			}
			framesToDeliver = framesToDeliver > 4096 ? 4096 : framesToDeliver;

			// check that we do not exceed the frames that we acctually have
			int remainingFrames = m_totalFrames - m_currPositionInFrames;
			if(remainingFrames <= 0) {
				std::cout << "done writing frames to pcm" << std::endl;
				break;
			}
			if(remainingFrames < framesToDeliver) {
				framesToDeliver = remainingFrames;
			}

			// write the frames to the buffer and update m_currPositionInFrames variable
			m_currPositionMutex.lock();
			{
				unsigned int offset = m_currPositionInFrames * m_channels * m_sampleChannelSizeBytes;
				int framesWritten = snd_pcm_writei(m_alsaPlaybackHandle, m_rawDataBuffer + offset, framesToDeliver);
				if( framesWritten < 0) {
					std::cerr << "write failed (" << snd_strerror(framesWritten) << ")" << std::endl;
				}
				m_currPositionInFrames += framesWritten;				
			}
			m_currPositionMutex.unlock();

		}

	}

	unsigned int SingleFilePlayer::getPositionInMs() {
		int err;
	 	snd_pcm_sframes_t delay;
	 	snd_pcm_sframes_t posInFrames;

	 	m_playStatusMutex.lock();
	 	if(m_playStatus != Playing) {
	 		m_playStatusMutex.unlock();
	 		return -1;
	 	}
	 	m_playStatusMutex.unlock();


 		m_currPositionMutex.lock();
 		{
 			if( (err = snd_pcm_delay(m_alsaPlaybackHandle, &delay)) < 0) {
 				std::cerr << "cannot query current offset in buffer " << std::endl;
 			}		
 			posInFrames = m_currPositionInFrames - delay; 			
 		}
		m_currPositionMutex.unlock();

		return (posInFrames * 1000) / m_frameRate;
	}

	bool SingleFilePlayer::isPlaying() {
		int status = snd_pcm_state(m_alsaPlaybackHandle);
		return (status == SND_PCM_STATE_RUNNING) || (status == SND_PCM_STATE_PREPARED);
	}

	void SingleFilePlayer::startPlay(unsigned int positionInMs) {

		m_currPositionInFrames = (positionInMs / 1000.0) * m_frameRate; 
		if(m_currPositionInFrames > m_totalFrames) {
			m_currPositionInFrames = m_totalFrames;
		}

		stop();

		int err;
		std::stringstream errDesc;
		if( (err = snd_pcm_prepare(m_alsaPlaybackHandle)) < 0 ) {
			errDesc << "snd_pcm_prepare failed (" << snd_strerror(err) << ")";
			m_playStatusMutex.unlock();
			throw std::runtime_error(errDesc.str());
		}

		m_playStatus = PlayStatus::Playing;
		m_playingThread = new std::thread(&SingleFilePlayer::playLoopOnThread, this);
	}

	void SingleFilePlayer::stop() {
		m_playStatusMutex.lock();
		if(m_playStatus == Stopped) {
			m_playStatusMutex.unlock();			
			return;
		}
		m_playStatus = Stopping;
		m_playStatusMutex.unlock();

		m_playingThread->join();
		delete m_playingThread;
		m_playingThread = nullptr;
	}

	void SingleFilePlayer::initialize(const std::string &path, const std::string &fileName) {
		std::stringstream fileDirStream;

		std::string wavPath = path;
		char * absPathCharArr = realpath(path.c_str(), NULL);
		if(absPathCharArr != NULL) {
			wavPath = absPathCharArr;
			free(absPathCharArr);
		}
		fileDirStream << wavPath << "/" << fileName;
		m_fileToPlay = fileName;
		m_fullFileName = fileDirStream.str();
		initSndFile();	
		initAlsa();	
	}

	void SingleFilePlayer::initSndFile() {
		SndfileHandle sndFile = SndfileHandle(m_fullFileName);
		if(sndFile.error() != 0) {
			std::stringstream errorDesc;
			errorDesc << "The file '" << m_fullFileName << "' cannot be opened. error msg: '" << sndFile.strError() << "'";
			throw std::runtime_error(errorDesc.str());
		}

		// set the parameters from read from the SndFile and produce log messages

		// sample rate
		m_frameRate = sndFile.samplerate();
		std::cout << "Frame rate (samples per seconds) is: " << m_frameRate << std::endl;

		// channels
		m_channels = sndFile.channels();
		std::cout << "Number of channels in each sample is: " << m_channels << std::endl;

		int majorType = sndFile.format() & SF_FORMAT_TYPEMASK;
		int minorType = sndFile.format() & SF_FORMAT_SUBMASK;
		std::cout << "wav format. major: 0x" << std::hex << majorType << " minor: 0x" << std::hex << minorType << std::dec << std::endl;

		switch(minorType) {
			case SF_FORMAT_PCM_S8: 
				m_sampleChannelSizeBytes = 1;
				m_sampleType = SampleTypeSigned;
				break;
			case SF_FORMAT_PCM_16: 
				m_sampleChannelSizeBytes = 2;
				m_sampleType = SampleTypeSigned;
				break;
			case SF_FORMAT_PCM_24: 
				m_sampleChannelSizeBytes = 3;
				m_sampleType = SampleTypeSigned;
				break;
			case SF_FORMAT_PCM_32: 
				m_sampleChannelSizeBytes = 4;
				m_sampleType = SampleTypeSigned;
				break;
			case SF_FORMAT_FLOAT:
				m_sampleChannelSizeBytes = 4;
				m_sampleType = SampleTypeFloat;
				break;
			case SF_FORMAT_DOUBLE:
				m_sampleChannelSizeBytes = 8;
				m_sampleType = SampleTypeFloat;
				break;
			default:
				std::stringstream errDesc;
				errDesc << "wav file is in unsupported format. minor format as read from sndFile is: " << std::hex << minorType;
				throw std::runtime_error(errDesc.str());
		}

		switch(majorType) {
			case SF_FORMAT_WAV:
				m_isEndianLittle = true;
				break;
			case SF_FORMAT_AIFF:
				m_isEndianLittle = false;
				break;
			default:
				std::stringstream errDesc;
				errDesc << "wav file is in unsupported format. major format as read from sndFile is: " << std::hex << majorType;
				throw std::runtime_error(errDesc.str());
		}

		std::cout << "each sample's channel is a " << m_sampleChannelSizeBytes << " bytes ";
		switch(m_sampleType) {
	    	case SampleTypeSigned: std::cout << "signed integer"; break;
	    	case SampleTypeUnsigned: std::cout << "unsigned integer"; break;
	    	case SampleTypeFloat: std::cout << "float"; break;
		}
		std::cout << " in " << (m_isEndianLittle ? "little" : "big") << " endian format" << std::endl;

		m_totalFrames = sndFile.frames();
		std::cout << "total frames in file: " << m_totalFrames << " and total time in ms is: " << (m_totalFrames * 1000) / m_frameRate << std::endl;

		int bufferSize = m_totalFrames * m_channels * m_sampleChannelSizeBytes;
		m_rawDataBuffer = new char[bufferSize];
		sndFile.readRaw(m_rawDataBuffer, bufferSize);

		m_sndFileInitialized = true;
	}

	bool SingleFilePlayer::GetFormatForAlsa(snd_pcm_format_t &outFormat) {
		switch(m_sampleType) {

			case SampleTypeSigned: {
				if(m_isEndianLittle) {
					switch(m_sampleChannelSizeBytes) {
						case 1: outFormat = SND_PCM_FORMAT_S8; return true;
						case 2: outFormat = SND_PCM_FORMAT_S16_LE; return true;
						case 3: outFormat = SND_PCM_FORMAT_S24_LE; return true;
						case 4: outFormat = SND_PCM_FORMAT_S32_LE; return true;
					}
				}
				else {
					switch(m_sampleChannelSizeBytes) {
						case 1: outFormat = SND_PCM_FORMAT_S8; return true;
						case 2: outFormat = SND_PCM_FORMAT_S16_BE; return true;
						case 3: outFormat = SND_PCM_FORMAT_S24_BE; return true;
						case 4: outFormat = SND_PCM_FORMAT_S32_BE; return true;
					}
				}
			}
			break;

			case SampleTypeUnsigned: {
				if(m_isEndianLittle) {
					switch(m_sampleChannelSizeBytes) {
						case 1: outFormat = SND_PCM_FORMAT_U8; return true;
						case 2: outFormat = SND_PCM_FORMAT_U16_LE; return true;
						case 3: outFormat = SND_PCM_FORMAT_U24_LE; return true;
						case 4: outFormat = SND_PCM_FORMAT_U32_LE; return true;
					}
				}
				else {
					switch(m_sampleChannelSizeBytes) {
						case 1: outFormat = SND_PCM_FORMAT_U8; return true;
						case 2: outFormat = SND_PCM_FORMAT_U16_BE; return true;
						case 3: outFormat = SND_PCM_FORMAT_U24_BE; return true;
						case 4: outFormat = SND_PCM_FORMAT_U32_BE; return true;
					}
				}
			}
			break;

			case SampleTypeFloat: {
				if(m_isEndianLittle) {
					switch(m_sampleChannelSizeBytes) {
						case 4: outFormat = SND_PCM_FORMAT_FLOAT_LE; return true;
						case 8: outFormat = SND_PCM_FORMAT_FLOAT64_LE; return true;
					}
				}
				else {
					switch(m_sampleChannelSizeBytes) {
						case 4: outFormat = SND_PCM_FORMAT_FLOAT_BE; return true;
						case 8: outFormat = SND_PCM_FORMAT_FLOAT64_BE; return true;
					}
				}			

			}
			break;

		}
		return false;
	}


	void SingleFilePlayer::initAlsa() {

		int err;
		std::stringstream errDesc;

		// const char *audioDevice = "plughw:0,0";
		const char *audioDevice = "default";
		if( (err = snd_pcm_open(&m_alsaPlaybackHandle, audioDevice, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
			errDesc << "cannot open audio device " << audioDevice << " (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		// set hw parameters

		snd_pcm_hw_params_t *hwParams;

		if( (err = snd_pcm_hw_params_malloc(&hwParams)) < 0 ) {
			errDesc << "cannot allocate hardware parameter structure (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		if( (err = snd_pcm_hw_params_any(m_alsaPlaybackHandle, hwParams)) < 0) {
			errDesc << "cannot initialize hardware parameter structure (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		if( (err = snd_pcm_hw_params_set_access(m_alsaPlaybackHandle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
			errDesc << "cannot set access type (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		snd_pcm_format_t alsaFormat;
		if(GetFormatForAlsa(alsaFormat) != true) {
			errDesc << "the wav format is not supported by this player of alsa";
			throw std::runtime_error(errDesc.str());
		}
		if( (err = snd_pcm_hw_params_set_format(m_alsaPlaybackHandle, hwParams, alsaFormat)) < 0) {
			errDesc << "cannot set sample format (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		if( (err = snd_pcm_hw_params_set_rate(m_alsaPlaybackHandle, hwParams, m_frameRate, 0)) < 0) {
			errDesc << "cannot set sample rate (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		if( (err = snd_pcm_hw_params_set_channels(m_alsaPlaybackHandle, hwParams, m_channels)) < 0) {
			errDesc << "cannot set channel count (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		if( (err = snd_pcm_hw_params(m_alsaPlaybackHandle, hwParams)) < 0) {
			errDesc << "cannot set alsa hw parameters (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		snd_pcm_hw_params_free(hwParams);
		hwParams = NULL;


		// set software parameters

		snd_pcm_sw_params_t *swParams;

		if( (err = snd_pcm_sw_params_malloc(&swParams)) < 0) {
			errDesc << "cannot allocate software parameters structure (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		if( (err = snd_pcm_sw_params_current(m_alsaPlaybackHandle, swParams)) < 0) {
			errDesc << "cannot initialize software parameters structure (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		if( (err = snd_pcm_sw_params_set_avail_min(m_alsaPlaybackHandle, swParams, 4096)) < 0) {
			errDesc << "cannot set minimum available count (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}	

		if( (err = snd_pcm_sw_params_set_start_threshold(m_alsaPlaybackHandle, swParams, 0U)) < 0) {
			errDesc << "cannot set start mode (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}	

		if( (err = snd_pcm_sw_params(m_alsaPlaybackHandle, swParams)) < 0) {
			errDesc << "cannot set software parameters (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}	

		snd_pcm_sw_params_free(swParams);
		swParams = NULL;

		if( (err = snd_pcm_prepare(m_alsaPlaybackHandle)) < 0) {
			errDesc << "cannot prepare audio interface for use (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}	

		m_alsaInitialized = true;

	}


}







