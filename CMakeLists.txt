cmake_minimum_required(VERSION 3.5)

project (wavplayeralsa)
set (CMAKE_CXX_STANDARD 11)
include_directories(src)

# include third party header only libraries
include_directories(thirdparty)

set (SOURCES 
	src/wavplayeralsa.cpp 
	src/alsa_frames_transfer.cc 
	src/web_sockets_api.cc
	src/http_api.cc
	src/audio_files_manager.cc
)

set(CMAKE_BUILD_TYPE Debug)

add_executable (wavplayeralsa ${SOURCES})
target_link_libraries(wavplayeralsa -lasound -lsndfile -lboost_system -lboost_filesystem -pthread)
