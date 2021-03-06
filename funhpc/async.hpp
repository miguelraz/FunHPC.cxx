#ifndef FUNHPC_ASYNC_HPP
#define FUNHPC_ASYNC_HPP

#include <cxx/cassert.hpp>
#include <cxx/invoke.hpp>
#include <funhpc/rexec.hpp>
#include <funhpc/rptr.hpp>
#include <qthread/future.hpp>

#include <cereal/types/tuple.hpp>

#include <tuple>
#include <type_traits>

namespace funhpc {

// rlaunch /////////////////////////////////////////////////////////////////////

enum class rlaunch : unsigned {
  async = static_cast<unsigned>(qthread::launch::async),
  deferred = static_cast<unsigned>(qthread::launch::deferred),
  sync = static_cast<unsigned>(qthread::launch::sync),
  detached = static_cast<unsigned>(qthread::launch::detached),
};

inline constexpr rlaunch operator~(rlaunch a) {
  return static_cast<rlaunch>(~static_cast<unsigned>(a));
}

inline constexpr rlaunch operator&(rlaunch a, rlaunch b) {
  return static_cast<rlaunch>(static_cast<unsigned>(a) &
                              static_cast<unsigned>(b));
}
inline constexpr rlaunch operator|(rlaunch a, rlaunch b) {
  return static_cast<rlaunch>(static_cast<unsigned>(a) |
                              static_cast<unsigned>(b));
}
inline constexpr rlaunch operator^(rlaunch a, rlaunch b) {
  return static_cast<rlaunch>(static_cast<unsigned>(a) ^
                              static_cast<unsigned>(b));
}

inline rlaunch &operator&=(rlaunch &a, rlaunch b) { return a = a & b; }
inline rlaunch &operator|=(rlaunch &a, rlaunch b) { return a = a | b; }
inline rlaunch &operator^=(rlaunch &a, rlaunch b) { return a = a ^ b; }

namespace detail {
// Convert bitmask to a specific policy
/*gcc constexpr*/ inline rlaunch decode_policy(rlaunch policy) {
  if ((policy | rlaunch::async) == rlaunch::async)
    return rlaunch::async;
  if ((policy | rlaunch::deferred) == rlaunch::deferred)
    return rlaunch::deferred;
  if ((policy | rlaunch::sync) == rlaunch::sync)
    return rlaunch::sync;
  if ((policy | rlaunch::detached) == rlaunch::detached)
    return rlaunch::detached;
  return rlaunch::async;
}

// Convert policy to a local policy
constexpr qthread::launch local_policy(rlaunch policy) {
  return static_cast<qthread::launch>(policy);
}
}

// async ///////////////////////////////////////////////////////////////////////

namespace detail {
template <typename R> struct set_result : std::tuple<> {
  void operator()(rptr<qthread::promise<R>> rpres, R &&res) const {
    auto pres = rpres.get_ptr();
    pres->set_value(std::move(res));
    delete pres;
  }
};
template <> struct set_result<void> : std::tuple<> {
  void operator()(rptr<qthread::promise<void>> rpres) const {
    auto pres = rpres.get_ptr();
    pres->set_value();
    delete pres;
  }
};

template <typename R> struct continued : std::tuple<> {
  template <typename F, typename... Args>
  void operator()(rptr<qthread::promise<R>> rpres, F &&f, Args &&... args) {
    rexec(rpres.get_proc(), set_result<R>(), rpres,
          cxx::invoke(std::forward<F>(f), std::forward<Args>(args)...));
  }
};
template <> struct continued<void> : std::tuple<> {
  template <typename F, typename... Args>
  void operator()(rptr<qthread::promise<void>> rpres, F &&f, Args &&... args) {
    cxx::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    rexec(rpres.get_proc(), set_result<void>(), rpres);
  }
};
}

template <typename F, typename... Args,
          typename R = std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>>
qthread::future<R> async(rlaunch policy, std::ptrdiff_t dest, F &&f,
                         Args &&... args) {
  if (dest == rank())
    return qthread::async(detail::local_policy(policy), std::forward<F>(f),
                          std::forward<Args>(args)...);
  auto pol = detail::decode_policy(policy);
  switch (pol) {
  case rlaunch::async:
  case rlaunch::sync: {
    auto pres = new qthread::promise<R>;
    auto fres = pres->get_future();
    rexec(dest, detail::continued<R>(), rptr<qthread::promise<R>>(pres),
          std::forward<F>(f), std::forward<Args>(args)...);
    if (pol == rlaunch::sync)
      fres.wait();
    return fres;
  }
  case rlaunch::deferred: {
    return qthread::async(qthread::launch::deferred,
                          [dest](auto &&f, auto &&... args) {
                            return async(rlaunch::async, dest, std::move(f),
                                         std::move(args)...)
                                .get();
                          },
                          std::forward<F>(f), std::forward<Args>(args)...);
  }
  case rlaunch::detached: {
    rexec(dest, std::forward<F>(f), std::forward<Args>(args)...);
    return qthread::future<R>();
  }
  }
  __builtin_unreachable();
}

template <typename F, typename... Args,
          typename R = std::decay_t<
              cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>>
qthread::future<R> async(rlaunch policy,
                         qthread::future<std::ptrdiff_t> &&fdest, F &&f,
                         Args &&... args) {
  cxx_assert(fdest.valid());
  if (fdest.ready())
    return async(policy, fdest.get(), std::forward<F>(f),
                 std::forward<Args>(args)...);
  return qthread::async(
      detail::local_policy(policy),
      [fdest = std::move(fdest)](auto &&f, auto &&... args) mutable {
        return async(rlaunch::sync, fdest.get(), std::move(f),
                     std::move(args)...)
            .get();
      },
      std::forward<F>(f), std::forward<Args>(args)...);
}
}

#define FUNHPC_ASYNC_HPP_DONE
#endif // #ifdef FUNHPC_ASYNC_HPP
#ifndef FUNHPC_ASYNC_HPP_DONE
#error "Cyclic include dependency"
#endif
