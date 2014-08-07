#ifndef RPC_GLOBAL_SHARED_PTR_FWD_HH
#define RPC_GLOBAL_SHARED_PTR_FWD_HH

#include "rpc_global_ptr_fwd.hh"

#include "cxx_utils.hh"

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <utility>

namespace rpc {

// A global shared pointer

// Layout:
//
// Pointer:
//    - ptr<M>:               manager
//    - maybe: global_ptr<M>: owner
//    - maybe: shared_ptr<T>: pointer (if local)
//
// Manager:
//    - atomic<ptrdiff_t>: reference count
//    - global_ptr<M>:     owner (may point to itself)
//    - global_ptr<T>:     pointer
//    - shared_ptr<T>:     pointer (if local)

class global_manager_base {
  std::atomic<ptrdiff_t> refcount;
  global_ptr<global_manager_base> owner;

public:
  bool invariant() const {
    if (refcount < 0)
      return false;
    if (!owner)
      return false;
    if (owner.is_local() && owner.get() != this)
      return false;
    return true;
  }
  // Create object from scratch
  global_manager_base() : refcount(1), owner(this) {
    RPC_ASSERT(invariant() && refcount > 0);
  }
  // Create from another manager
  global_manager_base(const global_ptr<global_manager_base> &owner,
                      const global_ptr<global_manager_base> &other);
  virtual ~global_manager_base();

  global_manager_base(const global_manager_base &) = delete;
  global_manager_base(global_manager_base &&) = delete;
  global_manager_base &operator=(const global_manager_base &) = delete;
  global_manager_base &operator=(global_manager_base &&) = delete;

  global_ptr<global_manager_base> get_owner() const { return owner; }

  void incref() { ++refcount; }
  void decref() {
    if (--refcount == 0)
      delete this;
  }
};

template <typename T> class global_manager : public global_manager_base {
  global_ptr<T> gptr;
  std::shared_ptr<T> sptr;

public:
  bool invariant() const {
    if (!global_manager_base::invariant())
      return false;
    if (!bool(gptr))
      return false;
    if (bool(sptr) != gptr.is_local())
      return false;
    return true;
  }
  // Create object from scratch
  global_manager(const std::shared_ptr<T> &ptr)
      : global_manager_base(), gptr(ptr.get()), sptr(ptr) {
    RPC_ASSERT(invariant());
  }
  global_manager(std::shared_ptr<T> &&ptr)
      : global_manager_base(), gptr(ptr.get()), sptr(std::move(ptr)) {
    RPC_ASSERT(invariant());
  }
  // Create from another manager
  global_manager(const global_ptr<global_manager_base> &owner,
                 const global_ptr<global_manager_base> &other,
                 const global_ptr<T> &gptr)
      : global_manager_base(owner, other), gptr(gptr), sptr(nullptr) {
    assert(gptr);
    RPC_ASSERT(gptr.get_proc() == owner.get_proc());
    RPC_ASSERT(invariant());
  }
  virtual ~global_manager() { RPC_ASSERT(invariant()); }

  global_manager() = delete;
  global_manager(const global_manager &) = delete;
  global_manager(global_manager &&) = delete;
  global_manager &operator=(const global_manager &) = delete;
  global_manager &operator=(global_manager &&) = delete;

  const global_ptr<T> &get_global() const { return gptr; }
  const std::shared_ptr<T> &get_shared() const {
    RPC_ASSERT(sptr);
    return sptr;
  }
};

// Pointer
template <typename T> class global_shared_ptr {
  global_manager<T> *mgr;

  void swap(global_shared_ptr &other) { std::swap(mgr, other.mgr); }

public:
  typedef T element_type;
  typedef T value_type;

  operator bool() const { return bool(mgr); }

  const global_ptr<T> &get_global() const {
    static const global_ptr<T> null(nullptr);
    if (!*this)
      return null;
    return mgr->get_global();
  }

  int get_proc() const { return get_global().get_proc(); }
  bool proc_is_ready() const { return get_global().proc_is_ready(); }
  future<int> get_proc_future() const { return get_global().get_proc_future(); }
  bool is_local() const { return get_global().is_local(); }

  // Can only get local objects
  const std::shared_ptr<T> &get() const {
    RPC_ASSERT(is_local());
    static const std::shared_ptr<T> null(nullptr);
    if (!*this)
      return null;
    return mgr->get_shared();
  }

  bool invariant() const {
    // nullptr:
    if (!mgr)
      return true;
    // check manager
    if (!mgr->invariant())
      return false;
    return true;
  }

  // Create empty
  global_shared_ptr() : mgr(nullptr) { RPC_ASSERT(invariant()); }

  // Create from shared pointer
  global_shared_ptr(const std::shared_ptr<T> &ptr)
      : mgr(ptr ? new global_manager<T>(ptr) : nullptr) {
    RPC_ASSERT(invariant());
  }
  global_shared_ptr(std::shared_ptr<T> &&ptr)
      : mgr(ptr ? new global_manager<T>(std::move(ptr)) : nullptr) {
    RPC_ASSERT(invariant());
  }

  // Copy constructor and friends
  global_shared_ptr(const global_shared_ptr &other) : mgr(other.mgr) {
    RPC_ASSERT(other.invariant());
    if (mgr)
      mgr->incref();
    RPC_ASSERT(invariant());
  }
  global_shared_ptr(global_shared_ptr &&other) : global_shared_ptr() {
    RPC_ASSERT(other.invariant());
    RPC_ASSERT(invariant());
    swap(other);
    RPC_ASSERT(invariant());
  }
  global_shared_ptr &operator=(const global_shared_ptr &other) {
    RPC_ASSERT(invariant() && other.invariant());
    if (this != &other) {
      if (mgr)
        mgr->decref();
      mgr = other.mgr;
      if (mgr)
        mgr->incref();
    }
    RPC_ASSERT(invariant());
    return *this;
  }
  global_shared_ptr &operator=(global_shared_ptr &&other) {
    RPC_ASSERT(invariant() && other.invariant());
    if (mgr)
      mgr->decref();
    mgr = nullptr;
    swap(other);
    RPC_ASSERT(invariant());
    return *this;
  }

  ~global_shared_ptr() {
    RPC_ASSERT(invariant());
    if (mgr)
      mgr->decref();
    mgr = nullptr;
  }

  bool operator==(const global_shared_ptr &other) const {
    return get_global() == other.get_global();
  }
  bool operator!=(const global_shared_ptr &other) const {
    return !(*this == other);
  }

  T &operator*() const { return *get(); }
  auto operator -> () const -> decltype(this -> get()) { return get(); }

  future<std::shared_ptr<T> > make_local() const;

  std::ostream &output(std::ostream &os) const {
    // TODO: output mgr itself as well
    return os << mgr;
  }

private:
  friend class cereal::access;
  template <class Archive> void save(Archive &ar) const;
  template <class Archive> void load(Archive &ar);
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const global_shared_ptr<T> &ptr) {
  return ptr.output(os);
}

template <typename T, typename... As>
global_shared_ptr<T> make_global_shared(const As &... args) {
  return std::make_shared<T>(args...);
}
}

#define RPC_GLOBAL_SHARED_FWD_HH_DONE
#else
#ifndef RPC_GLOBAL_SHARED_FWD_HH_DONE
#error "Cyclic include dependency"
#endif
#endif // #ifndef RPC_GLOBAL_SHARED_PTR_FWD_HH
