#ifndef FUN_ARRAY_HPP
#define FUN_ARRAY_HPP

#include <adt/array.hpp>
#include <adt/dummy.hpp>
#include <cxx/invoke.hpp>
#include <fun/idtype.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fun {

// is_array

namespace detail {
template <typename> struct is_array : std::false_type {};
template <typename T, std::size_t N>
struct is_array<std::array<T, N>> : std::true_type {};
}

// traits

template <typename> struct fun_traits;
template <typename T, std::size_t N> struct fun_traits<std::array<T, N>> {
  template <typename U> using constructor = std::array<U, N>;
  typedef constructor<adt::dummy> dummy;
  typedef T value_type;

  static constexpr std::ptrdiff_t rank = 1;
  typedef adt::index_t<rank> index_type;
  typedef adt::idtype<adt::dummy> boundary_dummy;
};

// iotaMap

template <
    typename C, typename F, typename... Args,
    std::enable_if_t<detail::is_array<C>::value> * = nullptr,
    typename R = cxx::invoke_of_t<const F &, std::ptrdiff_t, const Args &...>,
    typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMap(const F &f, const adt::irange_t &inds, const Args &... args) {
  CR rs;
  constexpr std::ptrdiff_t s = rs.size();
  assert(inds.size() == s);
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, inds[i], args...);
  return rs;
}

namespace detail {
struct array_iotaMapMulti {
  template <typename F, typename... Args>
  auto operator()(std::ptrdiff_t i, F &&f, Args &&... args) const {
    return cxx::invoke(std::forward<F>(f), adt::set<adt::index_t<1>>(i),
                       std::forward<Args>(args)...);
  }
};
}

template <typename C, std::size_t D, typename F, typename... Args,
          std::enable_if_t<detail::is_array<C>::value> * = nullptr,
          typename R = cxx::invoke_of_t<F, adt::index_t<D>, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR iotaMapMulti(F &&f, adt::index_t<D> s, Args &&... args) {
  static_assert(D == 1, "");
  return iotaMap<C>(detail::array_iotaMapMulti(), 0, s[0], 1,
                    std::forward<F>(f), std::forward<Args>(args)...);
}

// fmap

template <typename F, typename T, std::size_t N, typename... Args,
          typename C = std::array<T, N>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, const std::array<T, N> &xs, Args &&... args) {
  constexpr std::ptrdiff_t s = N;
  CR rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], args...);
  return rs;
}

template <typename F, typename T, std::size_t N, typename... Args,
          typename C = std::array<T, N>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap(F &&f, std::array<T, N> &&xs, Args &&... args) {
  constexpr std::ptrdiff_t s = N;
  CR rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, std::move(xs[i]), args...);
  return rs;
}

template <typename F, typename T, std::size_t N, typename T2, std::size_t N2,
          typename... Args, typename C = std::array<T, N>,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmap2(F &&f, const std::array<T, N> &xs, const std::array<T2, N2> &ys,
         Args &&... args) {
  constexpr std::ptrdiff_t s = N;
  static_assert(N2 == s, "");
  CR rs;
#pragma omp simd
  for (std::ptrdiff_t i = 0; i < s; ++i)
    rs[i] = cxx::invoke(f, xs[i], ys[i], args...);
  return rs;
}

// fmapStencil

template <typename F, typename G, typename T, std::size_t N, typename BM,
          typename BP, typename... Args, typename C = std::array<T, N>,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR fmapStencil(F &&f, G &&g, const std::array<T, N> &xs, std::size_t bmask,
               BM bm, BP bp, Args &&... args) {
  static_assert(std::is_same<std::decay_t<BM>, B>::value, "");
  static_assert(std::is_same<std::decay_t<BP>, B>::value, "");
  constexpr std::ptrdiff_t s = N;
  CR rs;
  if (__builtin_expect(s == 1, false)) {
    rs[0] = cxx::invoke(std::forward<F>(f), xs[0], bmask, std::forward<BM>(bm),
                        std::forward<BP>(bp), std::forward<Args>(args)...);
  } else if (__builtin_expect(s > 1, true)) {
    rs[0] = cxx::invoke(f, xs[0], bmask & 0b01, bm, cxx::invoke(g, xs[1], 0),
                        args...);
#pragma omp simd
    for (std::ptrdiff_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::invoke(f, xs[i], 0b00, cxx::invoke(g, xs[i - 1], 1),
                          cxx::invoke(g, xs[i + 1], 0), args...);
    rs[s - 1] = cxx::invoke(f, xs[s - 1], bmask & 0b10,
                            cxx::invoke(g, xs[s - 2], 1), bp, args...);
  }
  return rs;
}

template <std::size_t D, typename F, typename G, typename T, std::size_t N,
          typename... Args, std::enable_if_t<D == 1> * = nullptr,
          typename CT = std::array<T, N>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename B = cxx::invoke_of_t<G, T, std::ptrdiff_t>,
          typename BCB = typename fun_traits<BC>::template constructor<B>,
          typename R = cxx::invoke_of_t<F, T, std::size_t, B, B, Args...>,
          typename CR = typename fun_traits<CT>::template constructor<R>>
CR fmapStencilMulti(F &&f, G &&g, const std::array<T, N> &xs, std::size_t bmask,
                    BCB bm, BCB bp, Args &&... args) {
  constexpr std::ptrdiff_t s = N;
  CR rs;
  if (__builtin_expect(s == 1, false)) {
    rs[0] =
        cxx::invoke(std::forward<F>(f), xs[0], bmask, mextract(std::move(bm)),
                    mextract(std::move(bp)), std::forward<Args>(args)...);
  } else if (__builtin_expect(s > 1, true)) {
    rs[0] = cxx::invoke(f, xs[0], bmask & 0b01, mextract(std::move(bm)),
                        cxx::invoke(g, xs[1], 0), args...);
#pragma omp simd
    for (std::ptrdiff_t i = 1; i < s - 1; ++i)
      rs[i] = cxx::invoke(f, xs[i], 0b00, cxx::invoke(g, xs[i - 1], 1),
                          cxx::invoke(g, xs[i + 1], 0), args...);
    rs[s - 1] =
        cxx::invoke(f, xs[s - 1], bmask & 0b10, cxx::invoke(g, xs[s - 2], 1),
                    mextract(std::move(bp)), args...);
  }
  return rs;
}

// head, last

template <typename T, std::size_t N>
constexpr const T &head(const std::array<T, N> &xs) {
  static_assert(!xs.empty(), "");
  return xs.front();
}

template <typename T, std::size_t N>
constexpr const T &last(const std::array<T, N> &xs) {
  static_assert(!xs.empty(), "");
  return xs.back();
}

template <typename T, std::size_t N> constexpr T &&head(std::array<T, N> &&xs) {
  static_assert(!xs.empty(), "");
  return std::move(xs.front());
}

template <typename T, std::size_t N> constexpr T &&last(std::array<T, N> &&xs) {
  static_assert(!xs.empty(), "");
  return std::move(xs.back());
}

// boundary

template <typename T, std::size_t N, typename CT = std::array<T, N>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename BCT = typename fun_traits<BC>::template constructor<T>>
BCT boundary(const std::array<T, N> &xs, std::ptrdiff_t i) {
  assert(i >= 0 && i < 2);
  return munit<BC>(i == 0 ? head(xs) : last(xs));
}

// boundaryMap

template <typename F, typename T, std::size_t N, typename... Args,
          typename CT = std::array<T, N>,
          typename BC = typename fun_traits<CT>::boundary_dummy,
          typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
          typename BCR = typename fun_traits<BC>::template constructor<R>>
BCR boundaryMap(F &&f, const std::array<T, N> &xs, std::ptrdiff_t i,
                Args &&... args) {
  return fmap(std::forward<F>(f), boundary(xs, i), i,
              std::forward<Args>(args)...);
}

// indexing

template <typename T, std::size_t N>
const T &getIndex(const std::array<T, N> &xs, std::ptrdiff_t i) {
  static_assert(!xs.empty(), "");
  return xs[i];
}

template <typename> class accumulator;
template <typename T, std::size_t N> class accumulator<std::array<T, N>> {
  std::array<T, N> data;

public:
  accumulator(std::ptrdiff_t n) : data(n) {}
  T &operator[](std::ptrdiff_t i) { return data[i]; }
  decltype(auto) finalize() { return std::move(data); }
};

// template <typename F, typename T, std::size_t N, typename... Args,
//           typename C = std::array<T, N>,
//           typename R = cxx::invoke_of_t<F, T, std::ptrdiff_t, Args...>,
//           typename CR = typename fun_traits<C>::template constructor<R>>
// CR fmapIndexed(F &&f, const std::array<T, N> &xs, Args &&... args) {
//   constexpr std::ptrdiff_t s = N;
//   CR rs;
// #pragma omp simd
//   for (std::ptrdiff_t i = 0; i < s; ++i)
//     rs[i] = cxx::invoke(f, xs[i], i, args...);
//   return rs;
// }

// foldMap

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, const std::array<T, N> &xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  constexpr std::ptrdiff_t s = N;
  R r(std::forward<Z>(z));
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], args...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename R = cxx::invoke_of_t<F &&, T, Args &&...>>
R foldMap(F &&f, Op &&op, Z &&z, std::array<T, N> &&xs, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  constexpr std::ptrdiff_t s = N;
  R r(std::forward<Z>(z));
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r),
                    cxx::invoke(f, std::move(xs[i]), args...));
  return r;
}

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename T2, std::size_t N2, typename... Args,
          typename R = cxx::invoke_of_t<F, T, T2, Args...>>
R foldMap2(F &&f, Op &&op, Z &&z, const std::array<T, N> &xs,
           const std::array<T2, N2> &ys, Args &&... args) {
  static_assert(std::is_same<cxx::invoke_of_t<Op, R, R>, R>::value, "");
  constexpr std::ptrdiff_t s = N;
  static_assert(N2 == s, "");
  R r(std::forward<Z>(z));
#pragma omp declare reduction(op : R : (                                       \
    omp_out = cxx::invoke(op, std::move(omp_out),                              \
                                        omp_in))) initializer(omp_priv(z))
#pragma omp simd reduction(op : r)
  for (std::ptrdiff_t i = 0; i < s; ++i)
    r = cxx::invoke(op, std::move(r), cxx::invoke(f, xs[i], ys[i], args...));
  return r;
}

// munit

template <typename C, typename T,
          std::enable_if_t<detail::is_array<C>::value> * = nullptr,
          typename R = std::decay_t<T>,
          typename CR = typename fun_traits<C>::template constructor<R>>
constexpr CR munit(T &&x) {
  CR rs;
  rs.fill(std::forward<T>(x));
  return rs;
}

// mjoin

template <typename T, std::size_t N, std::size_t N2,
          typename CT = std::array<T, N>>
CT mjoin(const std::array<std::array<T, N>, N2> &xss) {
  static_assert(N2 == N, "");
  if (N == 0)
    return CT();
  return xss[0];
}

template <typename T, std::size_t N, std::size_t N2,
          typename CT = std::array<T, N>>
CT mjoin(std::array<std::array<T, N>, N2> &&xss) {
  static_assert(N2 == N, "");
  if (N == 0)
    return CT();
  return std::move(xss[0]);
}

// mbind

template <typename F, typename T, std::size_t N, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
CR mbind(F &&f, const std::array<T, N> &xs, Args &&... args) {
  static_assert(detail::is_array<CR>::value, "");
  return mjoin(fmap(std::forward<F>(f), xs, std::forward<Args>(args)...));
}

template <typename F, typename T, std::size_t N, typename... Args,
          typename CR = cxx::invoke_of_t<F, T, Args...>>
CR mbind(F &&f, std::array<T, N> &&xs, Args &&... args) {
  static_assert(detail::is_array<CR>::value, "");
  return mjoin(
      fmap(std::forward<F>(f), std::move(xs), std::forward<Args>(args)...));
}

// mextract

template <typename T, std::size_t N>
constexpr const T &mextract(const std::array<T, N> &xs) {
  assert(!xs.empty());
  return xs[0];
}

template <typename T, std::size_t N>
constexpr T &&mextract(std::array<T, N> &&xs) {
  assert(!xs.empty());
  return std::move(xs[0]);
}

// mfoldMap

template <typename F, typename Op, typename Z, typename T, std::size_t N,
          typename... Args, typename C = std::array<T, N>,
          typename R = cxx::invoke_of_t<F, T, Args...>,
          typename CR = typename fun_traits<C>::template constructor<R>>
CR mfoldMap(F &&f, Op &&op, Z &&z, const std::array<T, N> &xs,
            Args &&... args) {
  return munit<CR>(foldMap(std::forward<F>(f), std::forward<Op>(op),
                           std::forward<Z>(z), xs,
                           std::forward<Args>(args)...));
}

// mzero

template <typename C, typename R,
          std::enable_if_t<detail::is_array<C>::value> * = nullptr,
          typename CR = std::array<R, 0>>
constexpr CR mzero() {
  static_assert(CR().empty(), "");
  return CR{};
}

// mempty

template <typename T, std::size_t N>
constexpr bool mempty(const std::array<T, N> &xs) {
  return xs.empty();
}

// msize

template <typename T, std::size_t N>
constexpr std::size_t msize(const std::array<T, N> &xs) {
  return xs.size();
}
}

#define FUN_ARRAY_HPP_DONE
#endif // #ifdef FUN_ARRAY_HPP
#ifndef FUN_ARRAY_HPP_DONE
#error "Cyclic include dependency"
#endif
