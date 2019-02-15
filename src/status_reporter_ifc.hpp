#ifndef __STATUS_REPORTER_IFC_H__
#define __STATUS_REPORTER_IFC_H__

#include <cstdint>
#include <boost/asio.hpp>

#include "status_update_msg.h"


namespace wavplayeralsa {

	class StatusReporterIfc : public StatusUpdateMsg {

	public:
		virtual void Configure(boost::asio::io_service *ioSerivce, uint16_t wsListenPort) = 0;

	};

	StatusReporterIfc *CreateStatusReporter();

}

#endif // __STATUS_REPORTER_IFC_H__