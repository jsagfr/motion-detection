cmake_minimum_required(VERSION 3.0)
project( MotionDetection )

find_package( Boost 1.40
  REQUIRED
  COMPONENTS program_options )
include_directories( ${Boost_INCLUDE_DIR} )

find_package( OpenCV 3.0.0 REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable( motion_detection motion_detection.cpp )
target_link_libraries( motion_detection ${OpenCV_LIBS} )
target_link_libraries( motion_detection ${Boost_LIBRARIES} )

