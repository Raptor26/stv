cmake_minimum_required(VERSION 3.23)

project(test_stv_communication)

find_package(Catch2 REQUIRED)
find_package(FakeIt REQUIRED)

add_executable(${PROJECT_NAME} test_serial_sender.cpp test_serial_parser.cpp
                               test_serial_parser_demux.cpp test_crsf.cpp
                               test_stvlink_parser.cpp test_crc16.cpp
                               test_stvlink_dispatcher.cpp
                               test_mavlink_v2_parser.cpp
                               test_mavlink_v2_sender.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Catch2::Catch2WithMain
                                              FakeIt::FakeIt-catch stv::stv)
catch_discover_tests(${PROJECT_NAME})

stv_add_pedantic_compile_options_and_features(${PROJECT_NAME})

if(CLANG_TIDY_ENABLE)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}")
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY
                                                     "${DO_CLANG_TIDY}")
  else()
    message(FATAL_ERROR "clang-tidy not found.")
  endif()
endif()

if(ASAN_ENABLE)
  target_compile_options(${PROJECT_NAME} PRIVATE ${ASAN_SANITIZER_FLAGS})
  target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)
endif()

# Behchmark
find_package(benchmark)
if(benchmark_FOUND)
  set(project_name_bench ${PROJECT_NAME}_bench)
  add_executable(${project_name_bench} bench_serial.cpp)
  target_link_libraries(${project_name_bench} PRIVATE benchmark::benchmark
                                                      stv::stv)

  stv_add_pedantic_compile_options_and_features(${project_name_bench})
endif()
