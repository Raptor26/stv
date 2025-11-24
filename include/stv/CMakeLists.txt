cmake_minimum_required(VERSION 3.23)

target_include_directories(${PROJECT_NAME}
                           INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/")

add_subdirectory(filters)
