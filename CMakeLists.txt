cmake_minimum_required(VERSION 3.5)

project (wavplayeralsa)
set (CMAKE_CXX_STANDARD 11)
include_directories(src)

# include third party header only libraries
include_directories(thirdparty)

set (SOURCES 
	src/wavplayeralsa.cpp 
	src/single_file_player.cpp 
	src/status_reporter.cpp
	src/player_req_http.cpp
)

set(CMAKE_BUILD_TYPE Debug)

add_executable (wavplayeralsa ${SOURCES})
target_link_libraries(wavplayeralsa -lasound -lsndfile -lboost_system -pthread)
