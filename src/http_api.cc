
#include "http_api.h"

#include <boost/filesystem.hpp>


using json = nlohmann::json;


namespace wavplayeralsa {

	void HttpApi::Initialize(std::shared_ptr<spdlog::logger> logger, boost::asio::io_service *io_service, PlayerActionsIfc *player_action_callback, uint16_t http_listen_port) {

		// set class members
		player_action_callback_ = player_action_callback;
		logger_ = logger;

	  	server_.config.port = http_listen_port;
	  	server_.io_service = std::shared_ptr<boost::asio::io_service>(io_service);
		server_.resource["^/api/available-files$"]["GET"] = std::bind(&HttpApi::OnGetAvailableFiles, this, std::placeholders::_1, std::placeholders::_2);
		server_.resource["^/api/current-song$"]["PUT"] = std::bind(&HttpApi::OnPutCurrentSong, this, std::placeholders::_1, std::placeholders::_2);
		server_.default_resource["GET"] = std::bind(&HttpApi::OnWebGet, this, std::placeholders::_1, std::placeholders::_2);
		server_.on_error = std::bind(&HttpApi::OnServerError, this, std::placeholders::_1, std::placeholders::_2);


		try {
	  		server_.start();
	  	}
	  	catch(const std::exception &e) {
	  		std::stringstream err_msg;
	    	err_msg << "http server 'start' on port " << http_listen_port << " failed, probably not able to bind to port. error msg: " << e.what();
	    	throw std::runtime_error(err_msg.str());
	  	}

	  	logger_->info("http server started on port {}", http_listen_port);
	}

	void HttpApi::WriteResponseBadRequest(std::shared_ptr<HttpServer::Response> response, const std::stringstream &err_stream)
	{
		std::string err_msg = err_stream.str();
		*response << "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: " << err_msg.size() << "\r\n\r\n" << err_msg;		
	  	logger_->error("http request failed. returning error string: {}", err_msg);
	}

	void HttpApi::WriteResponseSuccess(std::shared_ptr<HttpServer::Response> response, const std::stringstream &body_stream) {
		std::string body = body_stream.str();
		*response << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;		
	  	logger_->info("http request succeeded. returning msg: {}", body);
	}

	void HttpApi::WriteJsonResponseSuccess(std::shared_ptr<HttpServer::Response> response, const json &body_json) {
		std::string body = body_json.dump();
		*response << "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " << body.size() << "\r\n\r\n" << body;		
	  	logger_->info("http request succeeded. returning msg: {}", body);
	}

	void HttpApi::OnGetAvailableFiles(std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
		const std::list<std::string> fileIds = player_action_callback_->QueryFiles();
		WriteJsonResponseSuccess(response, fileIds);
	}

	void HttpApi::OnPutCurrentSong(std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
		std::string request_json_str = request->content.string();
		logger_->info("http received put request for current-song: {}", request_json_str);

		// parse to json
		json j;
		try {
			j = json::parse(request_json_str);
		}
		catch(json::exception &e) {
			std::stringstream err_stream;
			err_stream << "http request content is not a json string. error msg: '" << e.what() << "'";
			WriteResponseBadRequest(response, err_stream);
		    return;
		}

		// validate song name
		std::string file_id;
		if(j.find("file_id") != j.end()) {
			try {
				file_id = j["file_id"].get<std::string>();
			}
			catch(json::exception &e) {
				std::stringstream err_stream;
				err_stream << "cannot find valid value for 'file_id' in request json. error msg: '" << e.what() << "'";
				WriteResponseBadRequest(response, err_stream);
			    return;
			}
		}

		uint64_t start_offset_ms = 0;
		// use it only if it is found in the json
		if(j.find("start_offset_ms") != j.end()) {
			try {
				start_offset_ms = j["start_offset_ms"].get<uint64_t>();
			}
			catch(json::exception &e) {
				std::stringstream err_stream;
				err_stream << "cannot find valid value for 'start_offset_ms' in request json. error msg: '" << e.what() << "'";
				WriteResponseBadRequest(response, err_stream);
			    return;
			}
		}

		std::stringstream handler_msg;
		bool success;
		if(file_id.empty()) {
			success = player_action_callback_->StopPlayRequest(handler_msg);
		}
		else {
			success = player_action_callback_->NewSongRequest(file_id, start_offset_ms, handler_msg);	
		} 

		if(!success) {
			WriteResponseBadRequest(response, handler_msg);
		    return;			
		}

		WriteResponseSuccess(response, handler_msg);
	}

	void HttpApi::OnWebGet(std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
	    try {
	      auto web_root_path = boost::filesystem::canonical("web");
	      auto path = boost::filesystem::canonical(web_root_path / request->path);
	      // Check if path is within web_root_path
	      if(std::distance(web_root_path.begin(), web_root_path.end()) > std::distance(path.begin(), path.end()) ||
	         !std::equal(web_root_path.begin(), web_root_path.end(), path.begin()))
	        throw std::invalid_argument("path must be within root path");
	      if(boost::filesystem::is_directory(path))
	        path /= "index.html";

	      SimpleWeb::CaseInsensitiveMultimap header;

	      auto ifs = std::make_shared<std::ifstream>();
	      ifs->open(path.string(), std::ifstream::in | std::ios::binary | std::ios::ate);

	      if(*ifs) {
	        auto length = ifs->tellg();
	        ifs->seekg(0, std::ios::beg);

	        header.emplace("Content-Length", to_string(length));
	        response->write(header);

	        // Trick to define a recursive function within this scope (for example purposes)
	        class FileServer {
	        public:
	          static void read_and_send(const std::shared_ptr<HttpServer::Response> &response, const std::shared_ptr<std::ifstream> &ifs) {
	            // Read and send 128 KB at a time
	            static std::vector<char> buffer(131072); // Safe when server is running on one thread
	            std::streamsize read_length;
	            if((read_length = ifs->read(&buffer[0], static_cast<std::streamsize>(buffer.size())).gcount()) > 0) {
	              response->write(&buffer[0], read_length);
	              if(read_length == static_cast<std::streamsize>(buffer.size())) {
	                response->send([response, ifs](const SimpleWeb::error_code &ec) {
	                  if(!ec)
	                    read_and_send(response, ifs);
	                  else
	                    std::cerr << "Connection interrupted" << std::endl;
	                });
	              }
	            }
	          }
	        };
	        FileServer::read_and_send(response, ifs);
	      }
	      else
	        throw std::invalid_argument("could not read file");
	    }
	    catch(const std::exception &e) {
	      response->write(SimpleWeb::StatusCode::client_error_bad_request, "Could not open path " + request->path + ": " + e.what());
	    }

	}

	void HttpApi::OnServerError(std::shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code &ec)
	{
		// TODO: find what this error means and how to handle it (print? what values can ec get)
		// logger_->error("http server error");
	}

}