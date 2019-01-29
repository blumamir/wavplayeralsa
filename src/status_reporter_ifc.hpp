#ifndef __STATUS_REPORTER_IFC_H__
#define __STATUS_REPORTER_IFC_H__

#include <boost/asio.hpp>


namespace wavplayeralsa {

	class StatusReporterIfc {

	public:
		virtual void Configure(boost::asio::io_service *ioSerivce, uint16_t wsListenPort) = 0;
		virtual void UpdateStatus() = 0;
	};

	StatusReporterIfc *CreateStatusReporter();

}

#endif // __STATUS_REPORTER_IFC_H__