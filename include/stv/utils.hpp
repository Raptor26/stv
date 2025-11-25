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

} // namespace stv

#endif /* UTILS_HPP */
