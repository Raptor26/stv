/// @file latch.hpp
/// @author Mickle Isaev (mrraptor26@gmail.com)
///
/// @copyright (c) 2026 Gagaring
///
/// MIT License:
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the 'Software'), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#ifndef LATCH_HPP
#define LATCH_HPP

#include <utility>

namespace stv {

/// @brief Класс защелки (latch) — одноразовое событийное устройство.
///
/// Класс `latch` реализует поведение одноразовой защелки: при получении
/// входного сигнала `true` он фиксирует состояние в `true` и остается в этом
/// состоянии до первого вызова `read()`. После вызова `read()` состояние
/// автоматически сбрасывается в `false`. Последующие вызовы `update(true)` не
/// имеют эффекта, пока состояние не будет сброшено.
///
/// Используется для обнаружения и однократного потребления событий, например:
/// - фиксации флага готовности данных
/// - обнаружения единичного импульса
/// - управления прерываниями в системах с ограниченными ресурсами
///
/// @note Класс не является потокобезопасным. Для использования в многопоточной
/// среде
///       требуется внешняя синхронизация (например, через `stv::lock_guard`).
///
/// @see stv::lock_guard, stv::empty_mutex
class latch
{
  private:
    /// @brief Текущее состояние защелки.
    /// - `false`: защелка не сработала
    /// - `true`: защелка сработала и ожидает сброса
    bool latched_{false};

  public:
    /// @brief Неявное приведение к `bool` для удобства проверки состояния.
    ///
    /// Возвращает `true`, если защелка установлена (т.е. `read()` вернет
    /// `true`), и `false` в противном случае.
    ///
    /// @return `true`, если защелка активна, иначе `false`.
    explicit operator bool() { return read(); }

    /// @brief Обновляет состояние защелки на основе входного сигнала.
    ///
    /// Если входное значение `input` равно `true` и защелка еще не была
    /// установлена, состояние защелки переходит в `true`. Если защелка уже
    /// установлена, вызов не имеет эффекта.
    ///
    /// @param[in] input Новое входное значение. Если `true` и защелка не
    /// установлена — защелка срабатывает. Если `false` — состояние не меняется.
    void update(
        bool input)
    {
        if(input)
        {
            latched_ = true;
        }
    }

    /// @brief Считывает текущее состояние защелки и сбрасывает его в `false`.
    ///
    /// Этот метод является единственным способом сбросить защелку.
    /// После вызова `read()` защелка возвращается в неактивное состояние
    /// (`false`), и может быть снова сработана только при последующем вызове
    /// `update(true)`.
    ///
    /// @return `true`, если защелка была установлена (т.е. до вызова `read()`
    /// значение было `true`), `false` в противном случае.
    ///
    /// @note Метод атомарно считывает и сбрасывает состояние, что делает его
    /// идеальным для обработки однократных событий без потерь.
    bool read() { return std::exchange(latched_, false); }
};

} // namespace stv

#endif /* LATCH_HPP */
