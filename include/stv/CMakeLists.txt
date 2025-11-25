cmake_minimum_required(VERSION 3.23)

add_subdirectory(filters)

if(UTEST)
  add_subdirectory(tests)
  target_compile_definitions(${PROJECT_NAME} INTERFACE -DUNIT_TEST_ENABLE)
endif()
