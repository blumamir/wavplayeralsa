#ifndef WAVPLAYERALSA_ALSA_SERVICE_H__
#define WAVPLAYERALSA_ALSA_SERVICE_H__

#include "spdlog/spdlog.h"

namespace wavplayeralsa
{

    class IAlsaPlaybackService
    {

    public:
        virtual ~IAlsaPlaybackService() { }

    public:


    };

    class AlsaPlaybackServiceFactory
    {

    public:
        void Initialize(
            std::shared_ptr<spdlog::logger> logger,
            const std::string &audio_device
        );

    public:

        IAlsaPlaybackService *CreateAlsaPlaybackService(
            const std::string &full_file_name, 
            const std::string &file_id
        );


	private:
		std::shared_ptr<spdlog::logger> logger_;

    private:
        std::string audio_device_;

    };

}


#endif // WAVPLAYERALSA_ALSA_SERVICE_H__