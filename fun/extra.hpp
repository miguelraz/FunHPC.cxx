#ifndef FUN_EXTRA_HPP
#define FUN_EXTRA_HPP

#include <adt/extra.hpp>

#include <adt/dummy.hpp>
#include <adt/empty.hpp>
#include <adt/index.hpp>
#include <cxx/cassert.hpp>
#include <cxx/invoke.hpp>
#include <fun/fun_decl.hpp>

#include <sstream>
#include <type_traits>
#include <utility>

namespace fun {

// is_extra

namespace detail {
template <typename> struct is_extra : std::false_type {};
template <typename E, typename T>
struct is_extra<adt::extra<E, T>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T, typename E> struct fun_traits<adt::extra<E, T>> {
  template <typename U> using constructor = adt::extra<E, std::decay_t<U>>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = 0;
  typedef adt::index_t<rank> index_type;

  typedef dummy boundary_dummy;

  static constexpr std::size_t min_size() { return 0; }
  static constexpr std::size_t max_size() { return 0; }
};

// iotaMap

template <typename C, typename F, typename... Args,
          std::enable_if_t<detail::is_extra<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, std::ptrdiff_t, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(F &&f, const adt::irange_t &inds, Args &&... args) {
  cxx_assert(inds.empty());
  return CR();
}

// fmap

template <typename F, typename T, typename E, typename... Args,
          typename C = adt::extra<E, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const adt::extra<E, T> &xs, Args &&... args) {
  return CR{xs.get_extra()};
}

template <typename F, typename T, typename E, typename... Args,
          typename C = adt::extra<E, T>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, adt::extra<E, T> &&xs, Args &&... args) {
  return CR{std::move(xs.get_extra())};
}

template <typename F, typename T, typename E, typename T2, typename E2,
          typename... Args, typename C = adt::extra<E, T>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const adt::extra<E, T> &xs, const adt::extra<E2, T2> &ys,
         Args &&... args) {
  return CR{xs.get_extra()};
}

// foldMap

template <typename F, typename Op, typename Z, typename T, typename E,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, const adt::extra<E, T> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return std::forward<Z>(z);
}

template <typename F, typename Op, typename Z, typename T, typename E,
          typename T2, typename E2, typename... Args,
          typename R = cxx::invoke_of_t<F &&, T, T2, Args &&...>>
R foldMap2(F &&f, Op &&op, Z &&z, const adt::extra<E, T> &xs,
           const adt::extra<E2, T2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  return std::forward<Z>(z);
}

// dump

template <typename T, typename E> ostreamer dump(const adt::extra<E, T> &xs) {
  std::ostringstream os;
  os << "extra{(" << xs.get_extra() << ")}";
  return ostreamer(os.str());
}

// mbind

template <typename F, typename T, typename E, typename... Args,
          typename CR = std::decay_t<cxx::invoke_of_t<F, T, Args...>>>
constexpr CR mbind(F &&f, const adt::extra<E, T> &xs, Args &&... args) {
  static_assert(detail::is_extra<CR>::value, "");
  return CR{xs.get_extra()};
}

// mjoin

template <typename T, typename E, typename E2, typename CT = adt::extra<E, T>>
constexpr CT mjoin(const adt::extra<E, adt::extra<E2, T>> &xss) {
  return CT{xss.get_extra()};
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_extra<C>::value> * = nullptr,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR mzero() {
  return CR();
}

// mplus

template <typename T, typename E, typename... Ts, typename... Es,
          typename CT = adt::extra<E, T>>
constexpr CT mplus(const adt::extra<E, T> &xs,
                   const adt::extra<Es, Ts> &... yss) {
  return xs;
}

// mextra

template <typename T, typename E>
constexpr bool mextra(const adt::extra<E, T> &xs) {
  return true;
}

// msize

template <typename T, typename E>
constexpr std::size_t msize(const adt::extra<E, T> &xs) {
  return 0;
}
}

#define FUN_EXTRA_HPP_DONE
#endif // #ifdef FUN_EXTRA_HPP
#ifndef FUN_EXTRA_HPP_DONE
#error "Cyclic include dependency"
#endif
