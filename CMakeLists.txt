cmake_minimum_required(VERSION 3.0)
project( MotionDetection )

find_package(Qt5Core)

find_package( OpenCV 3.0.0 REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable( motion_detection motion_detection.cpp )
target_link_libraries( motion_detection ${OpenCV_LIBS} )
target_link_libraries( motion_detection Qt5::Core )

