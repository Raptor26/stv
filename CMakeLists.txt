cmake_minimum_required(VERSION 3.23)

project(
  stv
  VERSION 0.1.0
  LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(STV_IS_STANDALONE ON)

  # По умолчанию включаем UTEST только в standalone-режиме: удобно для локальной
  # отладки (тесты собираются сразу), но внешние проекты всё ещё могут
  # переопределить флаг явно через -DUTEST.
  if(NOT DEFINED UTEST)
    set(UTEST ON)
  endif()
endif()

include(property.cmake)

add_subdirectory(third_party/GSL SYSTEM)
add_subdirectory(third_party/etl SYSTEM)
add_subdirectory(third_party/fpm SYSTEM)
add_subdirectory(third_party/lwrb SYSTEM)
add_subdirectory(third_party/frozen SYSTEM)
add_subdirectory(third_party/HFSM2 SYSTEM)

if((STV_IS_STANDALONE OR UTEST) AND TARGET lwrb)
  target_compile_definitions(lwrb PUBLIC -DLWRB_DISABLE_ATOMIC)
endif()

add_library(${PROJECT_NAME} INTERFACE)
add_library(stv::stv ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

if(STV_IS_STANDALONE)
  target_compile_definitions(${PROJECT_NAME} INTERFACE STV_IS_STANDALONE)
endif()

# enable_testing() должен быть вызван до catch_discover_tests() в тестовых
# поддиректориях. Размещаем его перед подключением дерева include/stv/;
# third_party выше не регистрируются как цели CTest.
enable_testing()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/include/stv/)
target_include_directories(${PROJECT_NAME}
                           INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include/")

target_link_libraries(stv INTERFACE Microsoft.GSL::GSL etl::etl fpm lwrb_ex
                                    frozen::frozen hfsm2::hfsm2)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(
    ${PROJECT_NAME}
    INTERFACE -Wno-gnu-statement-expression-from-macro-expansion)
endif()
