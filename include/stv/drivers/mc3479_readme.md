# Драйвер акселерометра MC3479

Библиотека предоставляет высокоуровневый драйвер для работы с 3-осевым акселерометром MC3479 по шине I2C.

## Требования

- Компилятор с поддержкой C++20 (требуется `std::endian`, `std::byteswap`)
- Реализация интерфейса `stv::i2c_interface` для вашей платформы
- Библиотека ETL (для `etl::reverse_bits`, опционально)

## Интеграция в проект

1. Добавьте заголовочные файлы в ваш проект:
   ```
   stv/drivers/mc3479_types.hpp
   stv/drivers/mc3479_regs.hpp
   stv/drivers/mc3479_i2c.hpp
   stv/drivers/mc3479.hpp
   ```

2. Реализуйте интерфейс `stv::i2c_interface` для вашей платформы (HAL/RTOS/bare-metal).

3. Подключите заголовок драйвера:
   ```cpp
   #include "stv/drivers/mc3479.hpp"
   ```

## Пример использования

```cpp
#include "stv/drivers/mc3479.hpp"
#include "stv/gyraccmag_types.hpp"
#include <cstdio>
#include <thread>
#include <chrono>

// -----------------------------------------------------------------------------
// Пример реализации интерфейса I2C для вашей платформы
// -----------------------------------------------------------------------------
class MyI2CInterface : public stv::i2c_interface {
public:
    bool read(std::uint8_t slave_addr, std::uint8_t reg_addr, 
              void* dst, std::size_t len) override {
        // TODO: Реализуйте чтение по I2C для вашей платформы
        // Пример для POSIX/Linux:
        // int fd = open("/dev/i2c-1", O_RDWR);
        // ioctl(fd, I2C_SLAVE, slave_addr);
        // write(fd, &reg_addr, 1);
        // read(fd, dst, len);
        return true; // Верните true при успехе
    }

    bool write(std::uint8_t slave_addr, std::uint8_t reg_addr, 
               std::uint8_t value) override {
        // TODO: Реализуйте запись по I2C для вашей платформы
        return true; // Верните true при успехе
    }
};

// -----------------------------------------------------------------------------
// Тип данных для нормированных измерений акселерометра
// -----------------------------------------------------------------------------
using acc_data_type = stv::acc<float, std::uint32_t>;

// -----------------------------------------------------------------------------
// Функция задержки для reset() (требуется для bare-metal/RTOS)
// -----------------------------------------------------------------------------
void delay_ms(std::uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    // Для bare-metal: вызовите вашу функцию задержки
    // HAL_Delay(ms);
}

// -----------------------------------------------------------------------------
// Основная программа
// -----------------------------------------------------------------------------
int main() {
    printf("MC3479 Driver Example\n");
    printf("=====================\n\n");

    // 1. Создаём объект I2C-интерфейса
    MyI2CInterface i2c_impl;

    // 2. Проверяем наличие датчика на шине (Chip ID = 0xA4)
    if (!stv::mc3479<acc_data_type>::is_detected(
            setup.i2c, setup.i2c_addr)) {
        printf("ERROR: MC3479 not detected on I2C bus!\n");
        return -1;
    }
    printf("MC3479 detected successfully!\n\n");
		
    // 3. Настраиваем параметры подключения датчика
    stv::mc3479_setup setup;
    setup.i2c = &i2c_impl;                                    // Указатель на I2C интерфейс
    setup.i2c_addr = stv::mc3479_i2c_setup::i2c_addr_connect_to_gnd; // 0x4C (A6=GND)
    // setup.i2c_addr = stv::mc3479_i2c_setup::i2c_addr_connect_to_vdd; // 0x6C (A6=VDD)

    // 4. Создаём объект драйвера
    stv::mc3479<acc_data_type> accelerometer(setup);

    // 5. Проверяем валидность транспорта
    if (!accelerometer) {
        printf("ERROR: Invalid I2C configuration!\n");
        return -1;
    }

    // 6. (Опционально) Выполняем сброс датчика
    // accelerometer.reset(delay_ms);

    // 7. Настраиваем регистры датчика
    stv::mc3479_regs_setup regs_setup;
    
    // Режим работы: wake (измерения активны)
    regs_setup.mode.state = stv::mc3479_state::wake;
    regs_setup.mode.wtd_neg = false;  // Watchdog отключён
    regs_setup.mode.wtd_pos = false;

    // Прерывания: отключены (опрос через статус-регистр)
    regs_setup.interrupt_enable.tilt_int_en = false;
    regs_setup.interrupt_enable.flip_int_en = false;
    regs_setup.interrupt_enable.anym_int_enable = false;
    regs_setup.interrupt_enable.shake_int_enable = false;
    regs_setup.interrupt_enable.tilt_35_int_enable = false;
    regs_setup.interrupt_enable.auto_clr_en = false;
    regs_setup.interrupt_enable.acq_int_en = false;

    // Частота дискретизации: 1000 Гц (по умолчанию)
    regs_setup.sample_rate.rate = stv::mc3479_sample_rate::hz_1000;

    // Motion-функции: отключены
    regs_setup.motion_control.tf_enable = false;
    regs_setup.motion_control.motion_latch = false;
    regs_setup.motion_control.anym_en = false;
    regs_setup.motion_control.shake_en = false;
    regs_setup.motion_control.tilt35_en = false;
    regs_setup.motion_control.z_axis_ort = false;
    regs_setup.motion_control.raw_proc_stat = false;
    regs_setup.motion_control.motion_reset = false;

    // Диапазон и фильтр: ±2g, LPF отключён
    regs_setup.range_and_scale_control.range = stv::mc3479_range::g2;
    regs_setup.range_and_scale_control.lpf_enable = false;
    regs_setup.range_and_scale_control.lpf = stv::mc3479_lpf::reserve;

    // 8. Инициализируем датчик с настройками
    if (!accelerometer.init(regs_setup)) {
        printf("ERROR: Failed to initialize MC3479!\n");
        return -1;
    }
    printf("MC3479 initialized successfully!\n\n");

    // 9. Цикл чтения данных
    printf("Reading accelerometer data (press Ctrl+C to stop)...\n\n");
    
    for (int i = 0; i < 10; ++i) {  // 10 измерений для примера
        // Читаем и нормализуем данные (в единицах g)
        auto acc = accelerometer.read_norm();

        printf("Sample #%d:\n", i + 1);
        printf("  X: %+8.4f g\n", acc.x);
        printf("  Y: %+8.4f g\n", acc.y);
        printf("  Z: %+8.4f g\n", acc.z);
        printf("  Timestamp: %u\n\n", acc.timestamp);

        // Задержка между измерениями (зависит от ODR)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 10. (Опционально) Получаем последнее измерение без чтения с датчика
    auto last_acc = accelerometer.get_acc();
    printf("Last cached measurement:\n");
    printf("  X: %+8.4f g, Y: %+8.4f g, Z: %+8.4f g\n", 
           last_acc.x, last_acc.y, last_acc.z);

    // 11. (Опционально) Переводим датчик в standby для экономии энергии
    // accelerometer.stop();

    printf("\nExample completed successfully!\n");
    return 0;
}
```

## Краткий справочник API

| Метод                    | Описание                                     |
| ------------------------ | -------------------------------------------- |
| `mc3479<AccType>(setup)` | Конструктор, принимает настройки I2C         |
| `operator bool()`        | Проверка валидности транспорта               |
| `is_detected(i2c, addr)` | Статический метод проверки наличия чипа      |
| `reset(delay_fn)`        | Выполняет сброс датчика с задержками         |
| `init(regs_setup)`       | Инициализация регистров конфигурации         |
| `read_raw()`             | Чтение сырых данных (counts)                 |
| `read_norm()`            | Чтение нормированных данных (g)              |
| `get_acc()`              | Получение последнего кэшированного измерения |
| `stop()`                 | Перевод в режим standby                      |
| `wake()`                 | Перевод в режим измерений (wake)             |
| `read_status_reg()`      | Чтение регистра статуса (флаги событий)      |

## Настройка диапазонов измерения

| Диапазон | Значение            | Чувствительность (LSB/g) |
| -------- | ------------------- | ------------------------ |
| ±2g      | `mc3479_range::g2`  | 16384                    |
| ±4g      | `mc3479_range::g4`  | 8192                     |
| ±8g      | `mc3479_range::g8`  | 4096                     |
| ±16g     | `mc3479_range::g16` | 2048                     |
| ±12g     | `mc3479_range::g12` | 2730                     |

## Частоты дискретизации (ODR)

| Частота | Значение                                |
| ------- | --------------------------------------- |
| 50 Гц   | `mc3479_sample_rate::hz_50`             |
| 100 Гц  | `mc3479_sample_rate::hz_100`            |
| 250 Гц  | `mc3479_sample_rate::hz_250`            |
| 500 Гц  | `mc3479_sample_rate::hz_500`            |
| 1000 Гц | `mc3479_sample_rate::hz_1000` (default) |
| 2000 Гц | `mc3479_sample_rate::hz_2000`           |

## Важные замечания

1. **Адрес I2C**: Зависит от состояния вывода A6:
   - GND → 0x4C
   - VDD → 0x6C

2. **Порядок инициализации**: 
   - `stop()` → настройка регистров → `wake()`
   - Метод `init()` выполняет это автоматически

3. **Endianess**: Драйвер автоматически обрабатывает byte-swap для big-endian платформ

4. **Bit-reversal**: При необходимости активации разворота битов определите макрос:
   ```cpp
   #define MC3479_BITS_REVERS 1
   ```

5. **Thread-safety**: Класс не является потокобезопасным. При использовании в многопоточной среде обеспечьте синхронизацию.

## Лицензия

SPDX-License-Identifier: MIT