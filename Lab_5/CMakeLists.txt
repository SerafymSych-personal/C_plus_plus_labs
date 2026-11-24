cmake_minimum_required(VERSION 3.16)
project(Lab_5 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Unicode entry point (wWinMain) and static runtime linking for MinGW-w64
if(MINGW)
	add_link_options(-municode -static)
endif()

add_executable(server WIN32 server.cpp)
add_executable(client1 WIN32 client1.cpp)
add_executable(client2 WIN32 client2.cpp)