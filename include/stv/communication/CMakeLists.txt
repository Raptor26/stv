cmake_minimum_required(VERSION 3.23)

project(
  stv_communication
  VERSION 0.1.0
  LANGUAGES C CXX)

add_library(${PROJECT_NAME} INTERFACE)
add_library(stv::communication ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
                           INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(${PROJECT_NAME} INTERFACE stv::stv)

if(UTEST)
  add_subdirectory(tests)
  target_compile_definitions(${PROJECT_NAME} INTERFACE -DUNIT_TEST_ENABLE)
endif()
