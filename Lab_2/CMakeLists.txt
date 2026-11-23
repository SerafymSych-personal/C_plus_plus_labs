cmake_minimum_required(VERSION 3.20)

project(Lab2_WinAPI CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app WIN32 main.cpp)

target_compile_options(app PRIVATE -municode -Wall -Wextra)
target_link_options(app PRIVATE -mwindows -municode -static)
target_link_libraries(app PRIVATE gdi32 user32 kernel32)