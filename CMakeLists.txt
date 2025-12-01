cmake_minimum_required(VERSION 3.23)

project(
  stv
  VERSION 0.1.0
  LANGUAGES C CXX)

include(property.cmake)

add_subdirectory(third_party/GSL SYSTEM)
add_subdirectory(third_party/leaf SYSTEM)
add_subdirectory(third_party/etl SYSTEM)
add_subdirectory(third_party/fpm SYSTEM)
add_subdirectory(third_party/lwrb SYSTEM)

add_library(${PROJECT_NAME} INTERFACE)
add_library(stv::stv ALIAS ${PROJECT_NAME})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/stv/)
target_include_directories(${PROJECT_NAME}
                           INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/")

target_link_libraries(stv INTERFACE Microsoft.GSL::GSL Boost::leaf etl::etl fpm
                                    lwrb_ex)
