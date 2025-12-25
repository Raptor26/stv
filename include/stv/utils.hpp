#ifndef UTILS_HPP
#define UTILS_HPP

namespace stv {

class non_copyable
{
  public:
    non_copyable() = default;

    non_copyable(const non_copyable &)            = delete;
    non_copyable &operator=(const non_copyable &) = delete;

    non_copyable(non_copyable &&)            = default;
    non_copyable &operator=(non_copyable &&) = default;

  protected:
    ~non_copyable() = default;
};

class non_movable
{
  public:
    non_movable() = default;

    non_movable(const non_movable &)            = default;
    non_movable &operator=(const non_movable &) = default;

    non_movable(non_movable &&)            = delete;
    non_movable &operator=(non_movable &&) = delete;

  protected:
    ~non_movable() = default;
};

/// ----------------------------------------------------------------------------
/// The macros below are useful for preventing optimization of structures and
/// classes with template parameters.
///
/// <pre>
/// {@code
/// STV_NO_PADDING_NO_OPTIMIZE_BEGIN
/// template <typename T>
/// struct MyStruct {
///   char a;
///   int b;
///   T t;
/// };
/// STV_NO_PADDING_NO_OPTIMIZE_END
/// }
/// </pre>
#if __GNUC__ || __clang__
    #define STV_NO_PADDING_NO_OPTIMIZE_BEGIN _Pragma("pack(push, 1)")
    #define STV_NO_PADDING_NO_OPTIMIZE_END   _Pragma("pack(pop)")
#elif _MSC_VER
    #define STV_NO_PADDING_NO_OPTIMIZE_BEGIN __pragma(pack(push, 1))
    #define STV_NO_PADDING_NO_OPTIMIZE_END   __pragma(pack(pop))
#else
    #error "Compiler does not support packing directives"
    #define STV_NO_PADDING_NO_OPTIMIZE_BEGIN
    #define STV_NO_PADDING_NO_OPTIMIZE_END
#endif

/// @brief Return true if the all variadic «Args» is true.
///
/// @see Шаблоны C++. Справочник разработчика, 2-е издание - 2018
/// 4.2. Выражения свертки.
///
/// @param[in] args Input boolean arguments sequence.
///
/// @return True if the all conditions in «Args» is valid. False in otherwise.
template<typename... Args>
auto all_true(
    Args... args)
{
    return (... && args);
}

/// @brief Логическое сложение. Возвращает true если хотя бы один из входных
/// аргументов равен true.
///
/// @param[in] args Входная последовательность аргументов.
///
/// @return Если хотя бы один из входных параметров равен true, метод вернет
/// true.
template<typename... Args>
auto one_true(
    Args... args)
{
    return (... || args);
}

} // namespace stv

#endif /* UTILS_HPP */
