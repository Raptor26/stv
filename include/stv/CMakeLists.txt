cmake_minimum_required(VERSION 3.23)

# Переменная ${ASAN_SANITIZER_FLAGS} содержит стандартные флаги санитайзера,
# которые вы можете добавить к своей цели с помощью команды ниже:
#[===[
if(ASAN_ENABLE)
  target_compile_options(${PROJECT_NAME} PRIVATE ${ASAN_SANITIZER_FLAGS})
  target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)
endif()
#]===]
set(ASAN_SANITIZER_FLAGS
    -fsanitize=address,undefined
    -fno-omit-frame-pointer # обязательно для хороших стек-трейсов
    -g # отладочная информация
    -O0 # отключить оптимизацию
    -fno-optimize-sibling-calls # улучшает стек-трейсы (опционально)
)

add_subdirectory(filters)
add_subdirectory(communication)
add_subdirectory(containers)
add_subdirectory(drivers)

if(UTEST)
  add_subdirectory(tests)
  target_compile_definitions(${PROJECT_NAME} INTERFACE -DUNIT_TEST_ENABLE)
endif()
