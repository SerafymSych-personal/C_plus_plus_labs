cmake_minimum_required(VERSION 3.16)
project(Lab_4 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MINGW)
	add_link_options(-municode -static)
endif()

# Server
add_executable(server WIN32 server.cpp)
target_link_libraries(server ws2_32)

# Client 1
add_executable(client1 WIN32 client1.cpp)
target_link_libraries(client1 ws2_32)

# Client 2
add_executable(client2 WIN32 client2.cpp)
target_link_libraries(client2 ws2_32)