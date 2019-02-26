#include "single_file_player.h"

#include <alsa/asoundlib.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdlib.h>
#include <sys/time.h>

namespace wavplayeralsa {

	SingleFilePlayer::SingleFilePlayer() : m_shouldBePlaying(false)
	{}
	

	SingleFilePlayer::~SingleFilePlayer() {

		if(m_alsaPlaybackHandle != NULL) {
			snd_pcm_close(m_alsaPlaybackHandle);
			m_alsaPlaybackHandle = NULL;
		}
	}

	const std::string &SingleFilePlayer::getFileToPlay() { 
		return m_fileToPlay; 
	}

	void SingleFilePlayer::checkSongStartTime() {
		int err;
		snd_pcm_sframes_t delay = 0;
		int64_t posInFrames = 0;

		if( (err = snd_pcm_delay(m_alsaPlaybackHandle, &delay)) < 0) {
			std::cerr << "cannot query current offset in buffer (" << snd_strerror(err) << ")" << std::endl;
		}		
		posInFrames = m_currPositionInFrames - delay; 			
		int64_t msSinceSongStart = ((posInFrames * (int64_t)1000) / (int64_t)m_frameRate);

		struct timeval tv;
		gettimeofday(&tv, NULL);
		uint64_t currTimeMsSinceEpoch = (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
		uint64_t songStartTimeMsSinceEphoc = (int64_t)currTimeMsSinceEpoch - msSinceSongStart;

		int64_t diffFromPrev = songStartTimeMsSinceEphoc - m_songStartTimeMsSinceEphoc;
		if(diffFromPrev > 1 || diffFromPrev < -1) {
			m_statusReporter->NewSongStatus(m_fileToPlay, songStartTimeMsSinceEphoc, 1.0);
			if(m_songStartTimeMsSinceEphoc > 0) {
				std::cout << diffFromPrev << "\t" << std::setfill('0') << std::setw(4) << (songStartTimeMsSinceEphoc % 10000) << "\t" << 
					std::setfill('0') << std::setw(4) << (currTimeMsSinceEpoch % 10000) << "\t" << 
					msSinceSongStart << "\t" << 
					posInFrames << "\t" << 
					m_currPositionInFrames << "\t" << 
					delay << 
					std::endl;		
			}
		}
		m_songStartTimeMsSinceEphoc = songStartTimeMsSinceEphoc;

	}

	void SingleFilePlayer::playLoopOnThread() {

		int err;

		while(1) {

			if(m_shouldBePlaying == false) {
				std::stringstream errDesc;
				if( (err = snd_pcm_drop(m_alsaPlaybackHandle)) < 0 ) {
					errDesc << "snd_pcm_drop failed (" << snd_strerror(err) << ")";
					throw std::runtime_error(errDesc.str());
				}
				return;
			}

			// TODO: if err == 0 -> timeout occured. 
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

			// we want to deliver as many frames as possible.
			// we can put framesToDeliver number of frames, but the buffer can only hold m_framesCapacityInBuffer frames
			framesToDeliver = framesToDeliver > m_framesCapacityInBuffer ? m_framesCapacityInBuffer : framesToDeliver;
			unsigned int bytesToDeliver = framesToDeliver * m_bytesPerFrame;

			// read the frames from the file. TODO: what if readRaw fails?
			char bufferForTransfer[TRANSFER_BUFFER_SIZE];
			bytesToDeliver = m_sndFile.readRaw(bufferForTransfer, bytesToDeliver);
			if(bytesToDeliver < 0) {
				std::cerr << "Failed reading raw frames from snd file. returned: " << bytesToDeliver << std::endl;
				continue;
			}
			if(bytesToDeliver == 0) {
				std::cout << "Done transfering snd frames to pcm." << std::endl;
				break;
			}

			int framesWritten = snd_pcm_writei(m_alsaPlaybackHandle, bufferForTransfer, framesToDeliver);
			if( framesWritten < 0) {
				std::cerr << "write failed (" << snd_strerror(framesWritten) << ")" << std::endl;
			}
			else {
				m_currPositionInFrames += framesWritten;
				if(framesWritten != framesToDeliver) {
					std::cerr << "transfered to alsa less frame then requested. framesToDeliver:" << framesToDeliver << " framesWritten: " << framesWritten << std::endl;
					m_sndFile.seek(m_currPositionInFrames, SEEK_SET);
				}
			}

			checkSongStartTime();
		}

		// we will be out of the loop if we finished writing frames to pcm.
		// now we only wait for the play to end

		while(true) {

			bool isCurrentlyPlaying = isAlsaStatePlaying();
			if(!isCurrentlyPlaying) {
				std::cout << "done playing current song. reached end of buffer and pcm is empty" << std::endl;
			}
			if(!isCurrentlyPlaying || m_shouldBePlaying == false) {
				std::stringstream errDesc;
				if( (err = snd_pcm_drop(m_alsaPlaybackHandle)) < 0 ) {
					errDesc << "snd_pcm_drop failed (" << snd_strerror(err) << ")";
					throw std::runtime_error(errDesc.str());
				}
				return;
			}

			std::chrono::milliseconds sleepTimeMs(5);
			std::this_thread::sleep_for(sleepTimeMs);
		}

	}

	bool SingleFilePlayer::isAlsaStatePlaying() {
		int status = snd_pcm_state(m_alsaPlaybackHandle);
		return (status == SND_PCM_STATE_RUNNING) || (status == SND_PCM_STATE_PREPARED);
	}

	void SingleFilePlayer::startPlay(uint32_t positionInMs) {

		m_currPositionInFrames = (positionInMs / 1000.0) * (double)m_frameRate; 
		if(m_currPositionInFrames > m_totalFrames) {
			m_currPositionInFrames = m_totalFrames;
		}
		m_sndFile.seek(m_currPositionInFrames, SEEK_SET);

		stop();

		int err;
		std::stringstream errDesc;
		if( (err = snd_pcm_prepare(m_alsaPlaybackHandle)) < 0 ) {
			errDesc << "snd_pcm_prepare failed (" << snd_strerror(err) << ")";
			throw std::runtime_error(errDesc.str());
		}

		m_shouldBePlaying = true;
		m_playingThread = new std::thread(&SingleFilePlayer::playLoopOnThread, this);
	}

	void SingleFilePlayer::stop() {
		std::cout << "stop is called on current alsa player (for the current song)." << std::endl;
		m_statusReporter->NoSongPlayingStatus();
		m_shouldBePlaying = false;
		if(m_playingThread != nullptr) {
			m_playingThread->join();
			delete m_playingThread;
			m_playingThread = nullptr;
		}
	}

	void SingleFilePlayer::initialize(const std::string &path, const std::string &fileName, StatusUpdateMsg *statusReporter) {

		m_statusReporter = statusReporter;

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

		m_sndFile = SndfileHandle(m_fullFileName);
		if(m_sndFile.error() != 0) {
			std::stringstream errorDesc;
			errorDesc << "The file '" << m_fullFileName << "' cannot be opened. error msg: '" << m_sndFile.strError() << "'";
			throw std::runtime_error(errorDesc.str());
		}

		// set the parameters from read from the SndFile and produce log messages

		// sample rate
		m_frameRate = m_sndFile.samplerate();
		std::cout << "Frame rate (samples per seconds) is: " << m_frameRate << std::endl;

		// channels
		m_channels = m_sndFile.channels();
		std::cout << "Number of channels in each sample is: " << m_channels << std::endl;

		int majorType = m_sndFile.format() & SF_FORMAT_TYPEMASK;
		int minorType = m_sndFile.format() & SF_FORMAT_SUBMASK;
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

		m_totalFrames = m_sndFile.frames();
		std::cout << "total frames in file: " << m_totalFrames << " and total time in ms is: " << (m_totalFrames * 1000) / m_frameRate << std::endl;

		m_bytesPerFrame = m_channels * m_sampleChannelSizeBytes;
		m_framesCapacityInBuffer = TRANSFER_BUFFER_SIZE / m_bytesPerFrame;

		uint64_t bufferSize = m_totalFrames * (uint64_t)m_channels * (uint64_t)m_sampleChannelSizeBytes;
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

	}


}







