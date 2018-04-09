cmake_minimum_required(VERSION 3.0)
project(LiveRTSPClient)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_CXX_FLAGS "-Wall -pthread")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

find_package(FFmpeg REQUIRED)
find_package(Live555 REQUIRED)
find_package(OpenCV 3 REQUIRED)

# executables
include_directories("inc")

add_executable(${PROJECT_NAME} ${SOURCE_FILES} src/main.cpp src/SDLVideoSink.cpp
        src/StreamClientState.cpp src/H265RTSPClient.cpp)

# FFmpeg
if (FFMPEG_FOUND)
    include_directories(${FFMPEG_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})
else(FFMPEG_FOUND)
    message(FATAL_ERROR "Can't find FFmpeg libs libavcodec, libavformat or libavutil.")
endif (FFMPEG_FOUND)

# Log4cpp
find_path(LOG4CPP_INCLUDE_DIR log4cpp/Category.hh)
if (LOG4CPP_INCLUDE_DIR)
    message(STATUS "Log4Cpp available")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_LOG4CPP")
    include_directories(${LOG4CPP_INCLUDE_DIR})

    find_library(LOG4CPP_LIBRARY log4cpp)
    target_link_libraries(${PROJECT_NAME} ${LOG4CPP_LIBRARY})
endif (LOG4CPP_INCLUDE_DIR)

# Live555
if (Live555_FOUND)
    message("Found Live555")
    foreach(Live555_module IN LISTS Live555_INCLUDE_DIRS)
        include_directories(${Live555_module})
    endforeach()
    target_link_libraries(${PROJECT_NAME} ${Live555_LIBRARIES})
else(Live555_FOUND)
    message(FATAL_ERROR "Can't find Live555 libraries")
endif(Live555_FOUND)

# OpenCV
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

# SDL
# Install libsdl2-dev and libsdl2-image-dev
INCLUDE(FindPkgConfig)

PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)

INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${SDL2_LIBRARIES} ${SDL2IMAGE_LIBRARIES})