#ifndef WAVPLAYERALSA_ALSA_SERVICE_H__
#define WAVPLAYERALSA_ALSA_SERVICE_H__

#include "spdlog/spdlog.h"

#include "player_events_ifc.h"

namespace wavplayeralsa
{

    class IAlsaPlaybackService
    {

    public:
        virtual ~IAlsaPlaybackService() { }

    public:
        virtual const std::string GetFileId() const = 0;
        virtual void Play(int64_t offset_in_ms) = 0;
        virtual bool Stop() = 0;

    };

    class AlsaPlaybackServiceFactory
    {

    public:
        void Initialize(
            std::shared_ptr<spdlog::logger> logger,
            PlayerEventsIfc *player_events_callback,
            const std::string &audio_device
        );

    public:

        IAlsaPlaybackService *CreateAlsaPlaybackService(
            const std::string &full_file_name, 
            const std::string &file_id,
            uint32_t play_seq_id
        );


	private:
		std::shared_ptr<spdlog::logger> logger_;

    private:
        PlayerEventsIfc *player_events_callback_;
        std::string audio_device_;

    };

}


#endif // WAVPLAYERALSA_ALSA_SERVICE_H__