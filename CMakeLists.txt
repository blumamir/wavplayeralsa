cmake_minimum_required(VERSION 3.5)

project (wavplayeralsa)
set (CMAKE_CXX_STANDARD 14)
include_directories(src)

find_package(Boost 1.65.1 REQUIRED COMPONENTS system filesystem)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
endif()

# include third party header only libraries
include_directories(thirdparty)
include_directories(thirdparty/mqtt_cpp)

set (SOURCES 
	src/wavplayeralsa.cpp 
	src/web_sockets_api.cc
	src/http_api.cc
	src/mqtt_api.cc
	src/audio_files_manager.cc
	src/current_song_controller.cc
	src/services/alsa_service.cc
	src/services/config_service.cc
)

set(CMAKE_BUILD_TYPE Debug)

add_executable (wavplayeralsa ${SOURCES})
target_link_libraries(wavplayeralsa -lasound -lsndfile ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} -pthread -luuid)
