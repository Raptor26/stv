#[[[
  Добавляет строгие опции компиляции и устанавливает стандарты языков

  Функция включает:
  - Строгие предупреждения компилятора (-Wall, -Wextra, -Wpedantic)
  - Режим ошибок для предупреждений (-Werror)
  - Дополнительные проверки (-Wshadow, -Wfatal-errors)
  - Современные стандарты языков (C++23, C17)

  Параметры:
    TARGET_NAME - имя существующей цели (executable или library)

  Пример:
    add_executable(my_app main.cpp)
    stv_add_pedantic_compile_options_and_features(my_app)
]]
function(stv_add_pedantic_compile_options_and_features TARGET_NAME)
  # Проверка существования цели
  if(NOT TARGET ${TARGET_NAME})
    message(FATAL_ERROR "Target '${TARGET_NAME}' does not exist")
  endif()

  # Строгие опции компиляции
  target_compile_options(
    ${TARGET_NAME}
    PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
            -Wall
            -Wextra
            -Wpedantic
            -Werror
            -Wshadow
            -Wfatal-errors
            -Wdouble-promotion
            -Wdocumentation
            -Wformat=2
            -Wformat-overflow
            -Wformat-truncation
            -Wundef
            -fno-common
            -Wconversion
            >
            $<$<CXX_COMPILER_ID:MSVC>:
            /W4
            /WX
            >)

  # Установка стандартов языков
  target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23 c_std_17)

  # Отключение расширений для лучшей портируемости
  set_target_properties(${TARGET_NAME} PROPERTIES CXX_EXTENSIONS OFF
                                                  C_EXTENSIONS OFF)
endfunction()
