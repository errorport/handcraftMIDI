cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" "cmake/")

project(handmarker)

set (CMAKE_COLOR_MAKEFILE ON)
set (CMAKE_CXX_FLAGS_DEBUG "-g -DQT_DEBUG -DDEBUG -D__RTMIDI_DEBUG__")
set (CMAKE_CXX_FLAGS_RELEASE "-O2 -DQT_NO_DEBUG -DNDEBUG")

add_definitions (-D__LINUX_ALSA__)
add_definitions (-DQT_NO_DEBUG_OUTPUT)

message (STATUS "Build configuration: ${CMAKE_BUILD_TYPE}")

set (handmarker
    ./handmarker.cpp
    ./rtmidi/RtMidi.h
)

find_package(Threads REQUIRED)
find_package(ALSA REQUIRED)
#find_package(RtMidi REQUIRED)
find_package(OpenCV REQUIRED)

#find_package(OpenMP)
#if (OPENMP_FOUND)
#    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
#    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#endif()

#set(OpenCV_LIBS "opencv_core opencv_imgproc opencv_calib3d opencv_video opencv_features2d opencv_ml opencv_highgui opencv_objdetect opencv_contrib opencv_legacy opencv_gpu")


include_directories(".")


add_executable(handmarker handmarker.cpp)
add_executable(MidiPortTest MidiPortTest.cpp)

target_link_libraries(handmarker ${OpenCV_LIBS})
target_link_libraries(handmarker "rtmidi")
target_link_libraries(MidiPortTest "rtmidi")
