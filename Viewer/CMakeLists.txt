cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(viewer)

find_package(PCL 1.7 REQUIRED)

include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

include_directories(${include})
link_directories(${include})

add_executable (viewer viewer.cpp include/util.h)
target_link_libraries (viewer ${PCL_LIBRARIES})
