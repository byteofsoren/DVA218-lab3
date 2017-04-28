cmake_minimum_required(VERSION 3.6)
project(dva218-lab3)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c server.c client.c handshake.c slidingwindow.c)
add_executable(dva218-lab3 ${SOURCE_FILES})