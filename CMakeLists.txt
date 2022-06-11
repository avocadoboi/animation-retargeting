cmake_minimum_required(VERSION 3.18.0)
project(AnimationRetargeting VERSION 1.0.0 LANGUAGES C CXX)

#---------------------------------------------------
# Library target.

add_library(animation_retargeting INTERFACE)

target_include_directories(animation_retargeting INTERFACE include/)

find_package(glm CONFIG REQUIRED)
target_link_libraries(animation_retargeting INTERFACE glm::glm)

#---------------------------------------------------
# Testing target.

add_subdirectory(testing)
