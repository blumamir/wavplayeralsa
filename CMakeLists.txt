cmake_minimum_required(VERSION 3.5)

project (wavplayeralsa)
set (CMAKE_CXX_STANDARD 11)
include_directories(src)

# zeromq dependency
find_package(cppzmq REQUIRED)
if(NOT cppzmq_FOUND)
	message(FATAL_ERROR "cppzmq wat not found. Probably not installed. Install it, then run CMake again")
endif()

# protobuf dependency
find_package(Protobuf REQUIRED)
if(NOT Protobuf_FOUND)
	message(FATAL_ERROR "Protobuf wat not found. Probably not installed. Install it, then run CMake again")
endif()

# nlohmann json library (single header only)
include_directories(thirdparty)

set (SOURCES 
	src/wavplayeralsa.cpp 
	src/single_file_player.cpp 
	src/position_reporter.cpp
	src/status_reporter.cpp
	src/player_req_http.cpp
	src/generated/position_report.pb.cc
	src/generated/player_command.pb.cc
)

set(CMAKE_BUILD_TYPE Debug)

add_executable (wavplayeralsa ${SOURCES})
target_link_libraries(wavplayeralsa -lasound -lsndfile -lboost_system -pthread cppzmq ${PROTOBUF_LIBRARY})
