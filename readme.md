# stv

> Файл для AI-агентов, работающих с проектом `stv`. Содержит фактическую информацию об архитектуре, сборке, стиле кода и тестировании.
> Проект: header-only C++23 библиотека для встраиваемых систем. Автор: Mickle Isaev. Лицензия: MIT. Версия: 0.2.0.
> Основной язык комментариев и документации в коде — русский; сообщения коммитов ведутся на русском (`CHANGELOG.md`).

---

## Project overview

`stv` — header-only C++ библиотека, ориентированная на микроконтроллерные/встраиваемые приложения.
Реализует типовые подсистемы:

- **Системные примитивы**: таймеры (`deadline_timer`), runtime-счётчик (`runtime`), мьютексы (`mutex_guard`), `latch`, `critical`, `callback_timer`.
- **Фильтры**: скользящее среднее (`filters/moving_average.hpp`).
- **Контейнеры**: `sim_buff` (`containers/simbuff.hpp`), обёртка над `lwrb` (`containers/lwrb.hpp`).
- **Коммуникации**: формирователь серийных сообщений с декораторами (`communication/serial_sender.hpp`, `serial_decorators.hpp`, `serial_parser.hpp`).
- **Драйверы I2C-сенсоров**: магнитометры и акселерометры (`drivers/qmc5883.hpp`, `qma6100.hpp`, `mmc56xx.hpp`, `mmc3630kj.hpp`, `mc3479.hpp`), а также унаследованная шина (`drivers/legacy_bus.hpp`).
- **Вспомогательные сущности**: концепты (`concepts.hpp`), type traits (`type_traits.hpp`), утилиты (`utils.hpp`), обёртки (`wrappers.hpp`), типы гироскопа/акселерометра/магнетометра (`gyraccmag_types.hpp`), индикатор состояния (`status_led.hpp`).

Все компоненты лежат в пространстве имён `stv`. Библиотека экспортируется как `INTERFACE`-цель CMake с алиасом `stv::stv`.

---

## Technology stack

- **Язык**: C++23, C17.
- **Система сборки**: CMake ≥ 3.23, генератор Ninja.
- **Компиляторы**: GCC (`gcc`/`g++`) и Clang (`clang`/`clang++`) — см. `CMakePresets.json`.
- **Статический анализ**: `clang-tidy` (`.clang-tidy`).
- **Форматирование**: `clang-format` (`.clang-format`, стиль на основе LLVM).
- **Контроль версий / коммиты**: Commitizen (`cz_conventional_commits`) через `.cz.json`, semver2, версия 0.2.0.
- **Тестирование**: Catch2 + FakeIt. Модульные тесты размещены рядом с кодом: `include/stv/<module>/tests/`.
- **Зависимости** (все в `third_party/` как git-subtree/submodule):
  - `GSL` (Microsoft.GSL)
  - `etl` (Embedded Template Library)
  - `fpm` (fixed-point math)
  - `lwrb` (lock-free ring buffer)
  - `frozen` (constexpr containers/algorithms)
- **Внешние пакеты для тестов**: `Catch2`, `FakeIt` — ищутся через `find_package(... REQUIRED)`.

---

## Repository structure

```text
stv/
├── CMakeLists.txt          # Корневая цель stv::stv и подключение third_party
├── CMakePresets.json       # Пресеты CMake
├── property.cmake          # Функция stv_add_pedantic_compile_options_and_features()
├── .clang-format           # Стиль форматирования
├── .clang-tidy             # Наборы проверок clang-tidy
├── .cz.json                # Commitizen конфигурация
├── CHANGELOG.md            # История изменений (русский язык)
├── include/stv/            # Весь публичный API (header-only)
│   ├── tests/              # Тесты базовых сущностей
│   ├── filters/            # Фильтры
│   ├── drivers/            # Драйверы сенсоров
│   ├── containers/         # Контейнеры
│   └── communication/      # Serial/коммуникации
├── third_party/            # Вендорные зависимости
└── build/                  # Выходные директории сборки
```

---

## Build and test commands

### Конфигурация, сборка и запуск тестов

Используйте CMake-пресеты:

```bash
# GCC
cmake --preset pc_unit_tests_gcc
cmake --build --preset pc_unit_tests_gcc
ctest --preset pc_unit_tests_gcc

# Clang
cmake --preset pc_unit_tests_clang
cmake --build --preset pc_unit_tests_clang
ctest --preset pc_unit_tests_clang
```

### clang-tidy

```bash
cmake --preset pc_unit_tests_clang_tidy
cmake --build --preset pc_unit_tests_clang_tidy
```

### Санитайзеры

Для запуска тестов с AddressSanitizer и UndefinedBehaviorSanitizer используйте dedicated-пресеты:

```bash
# GCC + sanitizers
cmake --preset pc_unit_tests_sanitizers_gcc
cmake --build --preset pc_unit_tests_sanitizers_gcc
ctest --preset pc_unit_tests_sanitizers_gcc

# Clang + sanitizers
cmake --preset pc_unit_tests_sanitizers_clang
cmake --build --preset pc_unit_tests_sanitizers_clang
ctest --preset pc_unit_tests_sanitizers_clang
```

> Ручная передача `-DASAN_ENABLE=ON` больше не требуется при использовании preset'ов.

### Standalone-режим

Если `stv` собирается как верхнеуровневый проект (не через `add_subdirectory`), CMake автоматически:

- Определяет макрос `STV_IS_STANDALONE`, который доступен в заголовках через `#ifdef STV_IS_STANDALONE`.
- Включает `UTEST=ON` по умолчанию, чтобы тесты собирались без дополнительных флагов.

```bash
# В корне репозитория stv
# Конфигурация автоматически определит standalone-режим и включит тесты
cmake --preset pc_unit_tests_gcc
```

При использовании как вложенной библиотеки макрос `STV_IS_STANDALONE` не определяется, а тесты собираются только при явном `-DUTEST=ON`.

### Встраивание библиотеки

Если `stv` используется как поддиректория, достаточно:

```cmake
add_subdirectory(stv)
target_link_libraries(your_target PRIVATE stv::stv)
```

При использовании в режиме `add_subdirectory` макрос `STV_IS_STANDALONE` не определяется.

---

## Code style guidelines

### Форматирование

- Форматировать код обязательно `clang-format` с `.clang-format`.
- Стиль основан на `LLVM`, `IndentWidth: 4`.
- Указатели и ссылки: `PointerAlignment: Right`, `ReferenceAlignment: Right`.
- Фигурные скобки: `BreakBeforeBraces: Custom` — открывающая скобка после класса/функции/управляющих конструкций переносится на новую строку.
- Короткие функции: `AllowShortFunctionsOnASingleLine: All`.
- `QualifierOrder: [inline, const, static, type]`.
- `PackConstructorInitializers: Never`.
- Вставка фигурных скобок: `InsertBraces: true`.

### Именование

Согласно `.clang-tidy` (`readability-identifier-naming`):

- Переменные, локальные и глобальные: `lower_case`.
- Константы: `lower_case`.
- Классы/структуры: `lower_case`.
- Приватные члены классов: суффикс `_` (например, `counter_`).
- Публичные/защищённые члены: без суффикса.
- Макросы: `UPPER_CASE`.

### Строгие опции компиляции

Функция `stv_add_pedantic_compile_options_and_features()` из `property.cmake` включает:

- GCC/Clang: `-Wall -Wextra -Wpedantic -Werror -Wshadow -Wfatal-errors -Wdouble-promotion -Wformat=2 -Wformat-overflow -Wformat-truncation -Wundef -fno-common -Wconversion -Wswitch-enum -Wfloat-equal -fstrict-aliasing -Wsign-conversion -Wcast-align -Wtype-limits -Wnon-virtual-dtor -Woverloaded-virtual`.
- Clang дополнительно: `-Wdocumentation`.
- MSVC: `/W4 /WX`.
- Стандарты: `cxx_std_23`, `c_std_17`.
- Расширения отключены (`CXX_EXTENSIONS OFF`, `C_EXTENSIONS OFF`).

### Стиль кода

- Весь публичный API документируется doxygen-комментариями (`/// @brief`, `@param`, `@return`, `@note`, `@warning`).
- Внутренние и публичные комментарии пишутся на русском языке (сохраняйте эту традицию).
- Заголовочные макросы защиты используются, но стиль не унифицирован (например, `RUNTIME_HPP`, `STVF_MOVING_AVERAGE_HPP`, `SERIAL_HPP`). При добавлении новых файлов предпочитайте префикс `STV_` + имя файла в верхнем регистре.
- Классы драйверов сенсоров наследуются от `stv::i2c_interface` и используют `STV_NO_PADDING_NO_OPTIMIZE_BEGIN/END` для регистровых структур.
- Для запрета копирования/перемещения используются `stv::non_copyable`, `stv::non_movable`, `stv::non_movable_non_copyable` из `utils.hpp`.

### clang-tidy

Включены почти все проверки из семейств `boost-*`, `bugprone-*`, `cert-*`, `clang-analyzer-*`, `cppcoreguidelines-*`, `google-*`, `hicpp-*`, `llvm-*`, `misc-*`, `modernize-*`, `performance-*`, `portability-*`, `readability-*` с рядом исключений (см. `.clang-tidy`).

- `WarningsAsErrors: '*'` — любое предупреждение трактуется как ошибка.
- `HeaderFilterRegex: '.*'` — проверяются все заголовки.
- Когнитивная сложность функций ограничена (`readability-function-cognitive-complexity.Threshold: 15`).

---

## Testing instructions

### Организация тестов

- Тесты расположены в `include/stv/<module>/tests/` рядом с тестируемым кодом.
- Каждый модуль собирает свои тесты через `CMakeLists.txt` внутри `tests/`.
- Переменная `UTEST` (`-DUTEST=ON`) включает тестовый режим; при этом определяется макрос `UNIT_TEST_ENABLE`:
  - `STV_VIRTUAL` становится `virtual`, что позволяет мокировать методы через FakeIt.
  - Некоторые приватные поля в фильтрах становятся публичными (`#ifdef UNIT_TEST_ENABLE`).

### Запуск

Запуск тестов выполняется через CMake-пресеты:

```bash
# GCC
ctest --preset pc_unit_tests_gcc

# Clang
ctest --preset pc_unit_tests_clang

# С санитайзерами
ctest --preset pc_unit_tests_sanitizers_gcc
ctest --preset pc_unit_tests_sanitizers_clang
```

Также собирается бенчмарк `test_stv_communication_bench` в `include/stv/communication/tests/`.

### Добавление новых тестов

- Создайте `.cpp` файл в `include/stv/<module>/tests/`.
- Добавьте файл в `add_executable(...)` соответствующего `tests/CMakeLists.txt`.
- Линкуйте с `Catch2::Catch2WithMain`, `FakeIt::FakeIt-catch` и `stv::stv`.
- Применяйте `stv_add_pedantic_compile_options_and_features()` к тестовой цели.
- Используйте `STV_VIRTUAL` для методов, которые нужно мокировать.

---

## Security considerations

- Библиотека предназначена для bare-metal/embedded: активно используются `reinterpret_cast`, прямые указатели, `memcpy` и плотная упаковка структур (`#pragma pack(1)`). Будьте аккуратны с выравниванием на ARM.
- `assert()` используется для проверки внутренних инвариантов; в релизных embedded-сборках обычно отключается (`NDEBUG`). Учитывайте это при добавлении критичных к безопасности проверок.
- Методы `filt()`, `setup()`, `inc()` и др. принимают флаг `is_isr` для корректной работы с мьютексами в контексте прерывания. Неверное использование может привести к deadlock'ам или состояниям гонки.
- `sim_buff` выделяет память через `std::allocator` и хранит сырые указатели; в embedded-контексте предпочтительнее статическое выделение или явный кастомный аллокатор.
- CRC в `start_frame_and_crc_16` — собственная простая реализация (`0xA001`); не используйте её как криптографическую контрольную сумму.
- При добавлении новых драйверов сенсоров:
  - Проверяйте корректность адресов регистров и размеров читаемых/записываемых блоков.
  - Используйте `STV_NO_PADDING_NO_OPTIMIZE_BEGIN/END` только там, где это необходимо для совпадения с layout регистров устройства.
  - Валидируйте `chip_id` и статусные регистры перед использованием измерений.

---

## Deployment and release

- Проект — библиотека, не имеет собственного процесса развёртывания.
- Версионирование ведётся через `.cz.json` (semver2).
- Обновление версии: `cz bump` обновит `CHANGELOG.md` и тег.
- Перед коммитом убедитесь, что проходит сборка с пресетом `pc_unit_tests_clang_tidy` и запускаются все тестовые бинарники.
