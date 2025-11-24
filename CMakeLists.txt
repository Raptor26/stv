cmake_minimum_required(VERSION 3.23)

project(
  stv
  VERSION 0.1.0
  LANGUAGES C CXX)

include(property.cmake)

add_subdirectory(third_parity/GSL SYSTEM)
add_subdirectory(third_parity/leaf SYSTEM)
add_subdirectory(third_parity/etl SYSTEM)

add_library(${PROJECT_NAME} INTERFACE)
add_library(stv::stv ALIAS ${PROJECT_NAME})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/stv/)
target_include_directories(${PROJECT_NAME}
                           INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/stv/")

target_link_libraries(stv INTERFACE Microsoft.GSL::GSL Boost::leaf etl::etl)
