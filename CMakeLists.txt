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

# ------------------------------------------------------------------------------
# Автоформатирование исходников stv через clang-format
# ------------------------------------------------------------------------------
#
# Для каждого C/C++ файла создаётся stamp-файл в build-директории. CMake
# сравнивает timestamp'ы: если исходник новее stamp — запускается clang-format и
# обновляется stamp. Так форматируются только изменённые файлы, а не все 60+ при
# каждой сборке.
#
# Форматирование привязано к цели stv через add_dependencies(), поэтому
# выполняется ДО компиляции любого зависимого кода.
# ------------------------------------------------------------------------------

find_program(CLANG_FORMAT_EXE NAMES clang-format)

if(CLANG_FORMAT_EXE)
  # CONFIGURE_DEPENDS: CMake автоматически переконфигурируется, если появятся
  # новые или удалятся старые файлы.
  file(
    GLOB_RECURSE
    _sources
    CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.c")

  set(_stamps)
  foreach(src ${_sources})
    # Относительный путь:
    cmake_path(RELATIVE_PATH src BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
               OUTPUT_VARIABLE rel)

    # Stamp зеркалит структуру исходников: build/format_stamps/include/stv/*
    set(stamp "${CMAKE_CURRENT_BINARY_DIR}/format_stamps/${rel}.stamp")
    cmake_path(GET stamp PARENT_PATH stamp_dir)
    file(MAKE_DIRECTORY "${stamp_dir}")

    # Форматируем исходник, затем обновляем stamp
    add_custom_command(
      OUTPUT "${stamp}"
      COMMAND "${CLANG_FORMAT_EXE}" -i --style=file "${src}"
      COMMAND "${CMAKE_COMMAND}" -E touch "${stamp}"
      DEPENDS "${src}"
      COMMENT "Formatting ${rel}"
      VERBATIM)

    list(APPEND _stamps "${stamp}")
  endforeach()

  # Цель без ALL: собирается только по зависимости (через add_dependencies ниже)
  add_custom_target(format_stv DEPENDS ${_stamps})
  add_dependencies(${PROJECT_NAME} format_stv)
endif()
