cmake_minimum_required(VERSION 3.23)

project(test_stv)

find_package(Catch2 REQUIRED)
find_package(FakeIt REQUIRED)

add_executable(
  ${PROJECT_NAME}
  test_deadline_timer.cpp
  test_latch.cpp
  test_mutex.cpp
  test_runtime.cpp
  test_utils.cpp
  test_critical.cpp
  test_while_timeout.cpp)

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
