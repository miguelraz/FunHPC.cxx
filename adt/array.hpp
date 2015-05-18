#ifndef ADT_ARRAY_HPP
#define ADT_ARRAY_HPP

#include <cxx/invoke.hpp>

#include <cereal/types/array.hpp>

#include <algorithm>
#include <array>
#include <ios>
#include <limits>
#include <type_traits>
#include <utility>

namespace std {

#define MAKEOP(op)                                                             \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(op std::declval<T>())>>         \
  auto operator op(const std::array<T, N> &x) {                                \
    std::array<R, N> r;                                                        \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = op x[i];                                                          \
    return r;                                                                  \
  }
MAKEOP(+)
MAKEOP(-)
MAKEOP(~)
MAKEOP(!)
#undef MAKEOP

#define MAKEOP(op)                                                             \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>>     \
  auto operator op(const std::array<T, N> &x, const std::array<U, N> &y) {     \
    std::array<R, N> r;                                                        \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x[i] op y[i];                                                     \
    return r;                                                                  \
  }                                                                            \
  template <                                                                   \
      typename T, std::size_t N, typename U,                                   \
      typename R =                                                             \
          std::decay_t<decltype(std::declval<T>() op std::declval<U>())>,      \
      std::enable_if_t<!std::is_base_of<std::ios_base, T>::value> * = nullptr> \
  auto operator op(const T &x, const std::array<U, N> &y) {                    \
    std::array<R, N> r;                                                        \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x op y[i];                                                        \
    return r;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<U>())>>     \
  auto operator op(const std::array<T, N> &x, const U &y) {                    \
    std::array<R, N> r;                                                        \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = x[i] op y;                                                        \
    return r;                                                                  \
  }
MAKEOP(+)
MAKEOP(-)
MAKEOP(*)
MAKEOP(/ )
MAKEOP(% )
MAKEOP(&)
MAKEOP(| )
MAKEOP (^)
MAKEOP(<< )
MAKEOP(>> )
MAKEOP(&&)
MAKEOP(|| )
// These are already provided by std::array, albeit with a different
// meaning; we thus provide functions eq, ne, lt, le, gt, ge instead
// MAKEOP(== )
// MAKEOP(!= )
// MAKEOP(< )
// MAKEOP(<= )
// MAKEOP(> )
// MAKEOP(>= )
#undef MAKEOP

#define MAKEOP(op)                                                             \
  template <typename T, std::size_t N, typename U>                             \
  std::array<T, N> &operator op(std::array<T, N> &x,                           \
                                const std::array<U, N> &y) {                   \
    for (std::size_t i = 0; i < N; ++i)                                        \
      x[i] op y[i];                                                            \
    return x;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U>                             \
  std::array<T, N> &operator op(std::array<T, N> &x, const U &y) {             \
    for (std::size_t i = 0; i < N; ++i)                                        \
      x[i] op y;                                                               \
    return x;                                                                  \
  }
MAKEOP(+= )
MAKEOP(-= )
MAKEOP(*= )
MAKEOP(/= )
MAKEOP(%= )
MAKEOP(&= )
MAKEOP(|= )
MAKEOP(^= )
MAKEOP(<<= )
MAKEOP(>>= )
#undef MAKEOP
}

namespace adt {

#define MAKEFUN(f)                                                             \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(std::f(std::declval<T>()))>>    \
  auto f(const std::array<T, N> &x) {                                          \
    std::array<R, N> r;                                                        \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = std::f(x[i]);                                                     \
    return r;                                                                  \
  }
MAKEFUN(abs)
#undef MAKEFUN

#define MAKEFUN(f)                                                             \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(                                \
                std::f(std::declval<T>(), std::declval<U>()))>>                \
  auto f(const std::array<T, N> &x, const std::array<U, N> &y) {               \
    std::array<R, N> r;                                                        \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = std::f(x[i], y[i]);                                               \
    return r;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(                                \
                std::f(std::declval<T>(), std::declval<U>()))>>                \
  auto f(const T &x, const std::array<U, N> &y) {                              \
    std::array<R, N> r;                                                        \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = std::f(x, y[i]);                                                  \
    return r;                                                                  \
  }                                                                            \
  template <typename T, std::size_t N, typename U,                             \
            typename R = std::decay_t<decltype(                                \
                std::f(std::declval<T>(), std::declval<U>()))>>                \
  auto f(const std::array<T, N> &x, const U &y) {                              \
    std::array<R, N> r;                                                        \
    for (std::size_t i = 0; i < N; ++i)                                        \
      r[i] = std::f(x[i], y);                                                  \
    return r;                                                                  \
  }
MAKEFUN(max)
MAKEFUN(min)
#undef MAKEFUN


template <typename T, std::size_t N>
/*gcc constexpr*/ inline auto array_zero() {
  std::array<T, N> r;
  r.fill(T(0));
  return r;
}

template <typename T, std::size_t N, typename U>
/*gcc constexpr*/ inline auto array_fill(const U &x) {
  std::array<T, N> r;
  r.fill(x);
  return r;
}

template <typename T, std::size_t N, std::size_t I>
/*gcc constexpr*/ inline auto array_dir() {
  static_assert(I >= 0 && I < N, "");
  std::array<T, N> r;
  r.fill(T(0));
  std::get<I>(r) = T(1);
  return r;
}

template <std::size_t I, typename T, std::size_t N>
auto rmdir(const std::array<T, N> &x) {
  static_assert(I >= 0 && I < N, "");
  std::array<T, N - 1> r;
  for (std::size_t i = 0; i < N - 1; ++i)
    r[i] = x[i + (i >= I)];
  return r;
}

template <std::size_t I, typename T, std::size_t N, typename U>
auto update(const std::array<T, N> &x, const U &y) {
  static_assert(I >= 0 && I < N, "");
  std::array<T, N> r(x);
  std::get<I>(r) = y;
  return r;
}

namespace detail {
template <typename T> struct numeric_limits {
  static constexpr T min() { return std::numeric_limits<T>::lowest(); }
  static constexpr T max() { return std::numeric_limits<T>::max(); }
};

#define MAKELIM(type)                                                          \
  template <> struct numeric_limits<type> {                                    \
    static constexpr type min() {                                              \
      return -std::numeric_limits<type>::infinity();                           \
    }                                                                          \
    static constexpr type max() {                                              \
      return +std::numeric_limits<type>::infinity();                           \
    }                                                                          \
  };
MAKELIM(float)
MAKELIM(double)
MAKELIM(long double)
#undef MAKELIM
}

#define MAKEREDOP(name, op, z)                                                 \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(std::declval<T>()               \
                                                   op std::declval<T>())>>     \
  R name(const std::array<T, N> &xs) {                                         \
    R r(z);                                                                    \
    for (const auto &x : xs)                                                   \
      r = std::move(r) op x;                                                   \
    return r;                                                                  \
  }
MAKEREDOP(sum, +, R(0))
MAKEREDOP(prod, *, R(1))
MAKEREDOP(all, &&, true)
MAKEREDOP(any, ||, false)
#undef MAKEREDOP

#define MAKEREDFUN(name, f, z)                                                 \
  template <typename T, std::size_t N,                                         \
            typename R = std::decay_t<decltype(                                \
                (f)(std::declval<T>(), std::declval<T>()))>>                   \
  R name(const std::array<T, N> &xs) {                                         \
    R r(z);                                                                    \
    for (const auto &x : xs)                                                   \
      r = (f)(std::move(r), x);                                                \
    return r;                                                                  \
  }
MAKEREDFUN(maxval, std::max, detail::numeric_limits<R>::min())
MAKEREDFUN(minval, std::min, detail::numeric_limits<R>::max())
#undef MAKEREDFUN
}

#define ADT_ARRAY_HPP_DONE
#endif // #ifdef ADT_ARRAY_HPP
#ifndef ADT_ARRAY_HPP_DONE
#error "Cyclic include dependency"
#endif
