cmake_minimum_required(VERSION 3.10)
project(stringswitch)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")

add_executable(demo main.cpp)
set_property(TARGET demo PROPERTY CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(TARGET demo PROPERTY CXX_STANDARD 17)
set_property(TARGET demo PROPERTY CMAKE_CXX_EXTENSIONS OFF)

