#ifndef UTILS_HPP
#define UTILS_HPP

namespace stv {

class NonCopyable
{
  public:
    NonCopyable()          = default;
    virtual ~NonCopyable() = default;

    NonCopyable(const NonCopyable &)            = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;

    NonCopyable(NonCopyable &&)            = default;
    NonCopyable &operator=(NonCopyable &&) = default;
};

class NonMovable
{
  public:
    NonMovable()          = default;
    virtual ~NonMovable() = default;

    NonMovable(const NonMovable &)            = default;
    NonMovable &operator=(const NonMovable &) = default;

    NonMovable(NonMovable &&)            = delete;
    NonMovable &operator=(NonMovable &&) = delete;
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
#if defined(__GNUC__) || defined(__clang__)

    #define STV_NO_PADDING_NO_OPTIMIZE_BEGIN _Pragma("pack(push, 1)")
    #define STV_NO_PADDING_NO_OPTIMIZE_END   _Pragma("pack(pop)")
#elifdef(_MSC_VER)
    #define STV_NO_PADDING_NO_OPTIMIZE_BEGIN __pragma(pack(push, 1))
    #define STV_NO_PADDING_NO_OPTIMIZE_END   __pragma(pack(pop))
#else
    #error "Compiler does not support packing directives"
    #define STV_NO_PADDING_NO_OPTIMIZE_BEGIN
    #define STV_NO_PADDING_NO_OPTIMIZE_END
#endif

} // namespace stv

#endif /* UTILS_HPP */
