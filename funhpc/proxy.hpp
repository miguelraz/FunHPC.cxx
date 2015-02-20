#ifndef FUNHPC_PROXY_HPP
#define FUNHPC_PROXY_HPP

#include <cxx/invoke.hpp>
#include <funhpc/async.hpp>
#include <funhpc/rexec.hpp>
#include <funhpc/serialize_shared_future.hpp>
#include <funhpc/shared_rptr.hpp>
#include <qthread/future.hpp>

#include <cereal/access.hpp>

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <type_traits>

namespace funhpc {

// proxy ///////////////////////////////////////////////////////////////////////

template <typename T> class proxy;

namespace detail {
template <typename T>
proxy<T> make_proxy_with_proc(std::ptrdiff_t proc,
                              qthread::future<proxy<T>> &&fptr);
}

template <typename T> class proxy {
  qthread::shared_future<shared_rptr<T>> robj;
  mutable std::ptrdiff_t proc;

  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(robj, proc); }

public:
  typedef T element_type;

  proxy() : proc(-1) { assert(invariant()); }

  proxy(const std::shared_ptr<T> &ptr) : proxy(shared_rptr<T>(ptr)) {}
  proxy(std::shared_ptr<T> &&ptr) : proxy(shared_rptr<T>(std::move(ptr))) {}

  proxy(const shared_rptr<T> &ptr) : proxy() {
    if (bool(ptr)) {
      robj = qthread::make_ready_future(ptr);
      proc = ptr.get_proc();
    }
    assert(invariant());
  }
  proxy(shared_rptr<T> &&ptr) : proxy() {
    if (bool(ptr)) {
      proc = ptr.get_proc();
      robj = qthread::make_ready_future(std::move(ptr));
    }
    assert(invariant());
  }

  proxy(const qthread::shared_future<std::shared_ptr<T>> &fptr) : proxy() {
    if (fptr.valid()) {
      if (fptr.ready()) {
        robj = qthread::make_ready_future(shared_rptr<T>(fptr.get()));
      } else {
        robj = qthread::async([fptr]() {
          auto ptr = fptr.get();
          assert(bool(ptr));
          return shared_rptr<T>(ptr);
        });
      }
      proc = rank();
    }
    assert(invariant());
  }
  proxy(qthread::shared_future<std::shared_ptr<T>> &&fptr) : proxy() {
    if (fptr.valid()) {
      if (fptr.ready()) {
        robj = qthread::make_ready_future(shared_rptr<T>(fptr.get()));
      } else {
        robj = qthread::async([fptr = std::move(fptr)]() mutable {
          auto ptr = fptr.get();
          assert(bool(ptr));
          return shared_rptr<T>(std::move(ptr));
        });
      }
      proc = rank();
    }
    assert(invariant());
  }
  proxy(qthread::future<std::shared_ptr<T>> &&fptr) : proxy() {
    if (fptr.valid()) {
      if (fptr.ready()) {
        robj = qthread::make_ready_future(shared_rptr<T>(fptr.get()));
      } else {
        robj = qthread::async([fptr = std::move(fptr)]() mutable {
          auto ptr = fptr.get();
          assert(bool(ptr));
          return shared_rptr<T>(std::move(ptr));
        });
      }
      proc = rank();
    }
    assert(invariant());
  }

  proxy(const qthread::shared_future<shared_rptr<T>> &fptr)
      : robj(fptr),
        proc(robj.valid() && robj.ready() ? robj.get().get_proc() : -1) {
    assert(invariant());
  }
  proxy(qthread::shared_future<shared_rptr<T>> &&fptr)
      : robj(std::move(fptr)),
        proc(robj.valid() && robj.ready() ? robj.get().get_proc() : -1) {
    assert(invariant());
  }
  proxy(qthread::future<shared_rptr<T>> &&fptr)
      : robj(std::move(fptr)),
        proc(robj.valid() && robj.ready() ? robj.get().get_proc() : -1) {
    assert(invariant());
  }

  proxy(const qthread::shared_future<proxy<T>> &fptr) : proxy() {
    if (fptr.valid()) {
      if (fptr.ready()) {
        *this = fptr.get();
      } else {
        robj = qthread::async([fptr]() {
          auto ptr = fptr.get();
          assert(bool(ptr));
          return ptr.robj.get();
        });
      }
    }
    assert(invariant());
  }
  proxy(qthread::shared_future<proxy<T>> &&fptr) : proxy() {
    if (fptr.valid()) {
      if (fptr.ready()) {
        *this = fptr.get();
      } else {
        robj = qthread::async([fptr = std::move(fptr)]() mutable {
          auto ptr = fptr.get();
          assert(bool(ptr));
          return ptr.robj.get();
        });
      }
    }
    assert(invariant());
  }
  proxy(qthread::future<proxy<T>> &&fptr) : proxy() {
    if (fptr.valid()) {
      if (fptr.ready()) {
        *this = fptr.get();
      } else {
        robj = qthread::async([fptr = std::move(fptr)]() mutable {
          auto ptr = fptr.get();
          assert(bool(ptr));
          return ptr.robj.get();
        });
      }
    }
    assert(invariant());
  }

private:
  friend proxy detail::make_proxy_with_proc<T>(std::ptrdiff_t proc,
                                               qthread::future<proxy> &&fptr);

  proxy(std::ptrdiff_t proc, qthread::future<proxy> &&fptr) : proxy() {
    assert(proc >= 0);
    if (fptr.valid()) {
      if (fptr.ready()) {
        *this = fptr.get();
        assert(proc == robj.get().get_proc());
      } else {
        robj = qthread::async([ proc, fptr = std::move(fptr) ]() mutable {
          auto ptr = fptr.get();
          assert(ptr.ready()); // ensure optimization works
          assert(proc == ptr.get_proc());
          return ptr.robj.get();
        });
        this->proc = proc;
      }
    }
    assert(invariant());
  }

public:
  proxy(const proxy &other) : robj(other.robj), proc(other.proc) {
    assert(invariant());
  }
  proxy(proxy &&other)
      : robj(std::move(other.robj)), proc(std::move(other.proc)) {
    assert(invariant());
  }
  proxy &operator=(const proxy &other) {
    robj = other.robj;
    proc = other.proc;
    return *this;
  }
  proxy &operator=(proxy &&other) {
    robj = std::move(other.robj);
    proc = std::move(other.proc);
    return *this;
  }
  void swap(proxy &other) {
    using std::swap;
    swap(robj, other.robj);
    swap(proc, other.proc);
  }
  void reset() { *this = proxy(); }

  bool invariant() const noexcept {
    if (!bool(*this))
      return !robj.valid() && proc < 0;
    return robj.valid() &&
           (proc >= 0 && robj.ready() ? proc == robj.get().get_proc() : true);
  }

  operator bool() const noexcept { return robj.valid(); }
  bool proc_ready() const noexcept {
    assert(bool(*this));
    return proc >= 0;
  }
  std::ptrdiff_t get_proc() const {
    assert(bool(*this));
    if (proc > 0)
      return proc;
    return proc = robj.get().get_proc();
  }
  const qthread::future<std::ptrdiff_t> &get_proc_future() const {
    assert(bool(*this));
    if (proc >= 0)
      return qthread::make_ready_future(proc);
    return qthread::async([robj = this->robj]() {
      return robj.get().get_proc();
    });
  }
  bool local() const {
    assert(bool(*this));
    return get_proc() == rank();
  }

  bool ready() const noexcept {
    assert(bool(*this));
    return robj.ready();
  }
  void wait() const {
    assert(bool(*this));
    robj.wait();
    if (proc < 0)
      proc = robj.get().get_proc();
  }

  const std::shared_ptr<T> &get_shared_ptr() const {
    static const std::shared_ptr<T> null;
    if (!bool(*this))
      return null;
    assert(local());
    return robj.get().get_shared_ptr();
  }
  const T &operator*() const {
    assert(bool(*this) && local());
    return *get_shared_ptr();
  }
  T &operator*() {
    assert(bool(*this) && local());
    return *get_shared_ptr();
  }
  const std::shared_ptr<T> &operator->() const { return get_shared_ptr(); }

  proxy make_local() const;

  bool operator==(const proxy &other) const {
    auto nt = bool(*this), no = bool(other);
    if (nt != no)
      return false;
    if (nt)
      return true;
    return robj.get() == other.robj.get();
  }
  bool operator!=(const proxy &other) const { return !(*this == other); }
  bool operator<=(const proxy &other) const {
    auto nt = bool(*this), no = bool(other);
    if (nt != no)
      return nt <= no;
    return robj.get() <= other.robj.get();
  }
};
template <typename T> void swap(proxy<T> &lhs, proxy<T> &rhs) { lhs.swap(rhs); }

namespace detail {
template <typename T>
proxy<T> make_proxy_with_proc(std::ptrdiff_t proc,
                              qthread::future<proxy<T>> &&fptr) {
  return proxy<T>(proc, std::move(fptr));
}
}

// make_proxy //////////////////////////////////////////////////////////////////

template <typename T, typename... Args>
proxy<T> make_local_proxy(Args &&... args) {
  return proxy<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

namespace detail {
// TODO: Check that args are move-constructed
template <typename T, typename... Args>
proxy<T> make_local_proxy_decayed(Args... args) {
  return proxy<T>(std::make_shared<T>(std::move(args)...));
}
}

template <typename T, typename... Args>
proxy<T> make_remote_proxy(std::ptrdiff_t dest, Args &&... args) {
  return detail::make_proxy_with_proc(
      dest, async(rlaunch::async | rlaunch::deferred, dest,
                  detail::make_local_proxy_decayed<T, std::decay_t<Args>...>,
                  std::forward<Args>(args)...));
}

// remote //////////////////////////////////////////////////////////////////////

namespace detail {
// TODO: Check that f and args are move-constructed
template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>
proxy<R> local_decayed(F f, Args... args) {
  return make_local_proxy<R>(cxx::invoke(std::move(f), std::move(args)...));
}
}

template <typename F, typename... Args,
          typename R = cxx::invoke_of_t<std::decay_t<F>, std::decay_t<Args>...>>
proxy<R> remote(std::ptrdiff_t dest, F &&f, Args &&... args) {
  return detail::make_proxy_with_proc(
      dest, async(rlaunch::async | rlaunch::deferred, dest,
                  detail::local_decayed<std::decay_t<F>, std::decay_t<Args>...>,
                  std::forward<F>(f), std::forward<Args>(args)...));
}

// make_local_shared_ptr ///////////////////////////////////////////////////////

namespace detail {
template <typename T>
std::shared_ptr<T> proxy_get_shared_ptr(const proxy<T> &rptr) {
  return rptr.get_shared_ptr();
}
}

template <typename T>
qthread::future<std::shared_ptr<T>>
make_local_shared_ptr(const proxy<T> &rptr) {
  assert(bool(rptr));
  if (rptr.proc_ready()) {
    if (rptr.local()) {
      if (rptr.ready())
        return qthread::make_ready_future(detail::proxy_get_shared_ptr(rptr));
      return qthread::async(detail::proxy_get_shared_ptr<T>, rptr);
    }
    return async(rlaunch::async | rlaunch::deferred, rptr.get_proc(),
                 detail::proxy_get_shared_ptr<T>, rptr);
  }
  return qthread::async([rptr]() {
    return async(rlaunch::async | rlaunch::deferred, rptr.get_proc(),
                 detail::proxy_get_shared_ptr<T>, rptr).get();
  });
}

template <typename T> proxy<T> proxy<T>::make_local() const {
  assert(bool(*this));
  return proxy(make_local_shared_ptr(*this));
}
}

#define FUNHPC_PROXY_HPP_DONE
#endif // #ifdef FUNHPC_PROXY_HPP
#ifndef FUNHPC_PROXY_HPP_DONE
#error "Cyclic include dependency"
#endif
