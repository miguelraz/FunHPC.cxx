#ifndef QTHREAD_MUTEX_HPP
#define QTHREAD_MUTEX_HPP

#include <qthread/qthread.hpp>
#include <qthread/qt_syscalls.h>

#include <cassert>

namespace qthread {

// mutex ///////////////////////////////////////////////////////////////////////

class mutex {
  syncvar mem;

public:
  mutex() noexcept {}
  mutex(const mutex &) = delete;
  mutex(mutex &&) = delete;
  ~mutex() { assert(mem.status()); }
  mutex &operator=(const mutex &) = delete;
  mutex &operator=(mutex &&) = delete;
  void lock() { mem.readFE(); }
  void unlock() {
    assert(!mem.status());
    mem.fill();
  }
};

// lock_guard //////////////////////////////////////////////////////////////////

template <typename M> class lock_guard {
  M &mtx;

public:
  explicit lock_guard(M &m) : mtx(m) { mtx.lock(); }
  lock_guard(const lock_guard &) = delete;
  lock_guard(lock_guard &&) = delete;
  lock_guard &operator=(const lock_guard &) = delete;
  lock_guard &operator=(lock_guard &&) = delete;
  ~lock_guard() { mtx.unlock(); }
};
}

#define QTHREAD_MUTEX_HPP_DONE
#endif // #ifndef QTHREAD_MUTEX_HPP
#ifndef QTHREAD_MUTEX_HPP_DONE
#error "Cyclic include dependency"
#endif
