cmake_minimum_required(VERSION 3.0)
project(testgui)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11  -g -fPIC -DHAVE_CONFIG_H ")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -g -fPIC")

SET( CMAKE_EXE_LINKER_FLAGS  "-g " )

include_directories("../hidapi/")
include_directories("/usr/include/fox-1.6/")


set(SOURCE_FILES  test.cpp ../linux/hid.c)

add_executable(testgui ${SOURCE_FILES})

target_link_libraries(testgui udev FOX-1.6)