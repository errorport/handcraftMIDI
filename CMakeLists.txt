cmake_minimum_required(VERSION 2.8)

project(handmarker)


find_package(OpenCV REQUIRED)

#set(OpenCV_LIBS "opencv_core opencv_imgproc opencv_calib3d opencv_video opencv_features2d opencv_ml opencv_highgui opencv_objdetect opencv_contrib opencv_legacy opencv_gpu")

add_executable(handmarker handmarker.cpp)

include_directories(".")

target_link_libraries(handmarker ${OpenCV_LIBS})
