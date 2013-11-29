#ifndef RPC_GLOBAL_SHARED_PTR_HH
#define RPC_GLOBAL_SHARED_PTR_HH

#include "rpc_call.hh"
#include "rpc_global_ptr.hh"
#include "rpc_global_shared_ptr_fwd.hh"
#include "rpc_server.hh"

#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>

namespace rpc {
  
  using std::atomic;
  using std::function;
  using std::ostream;
  
  
  
  inline void remove_ref(global_ptr<global_manager_t> manager)
  {
    global_manager_t::remove_ref(manager.get());
  }
  struct remove_ref_action:
    public action_impl<remove_ref_action,
                       wrap<decltype(remove_ref), remove_ref>>
  {
  };
  
  inline void add_remove_ref(global_ptr<global_manager_t> owner,
                             global_ptr<global_manager_t> sender_manager)
  {
    global_manager_t::add_ref(owner.get());
    apply(sender_manager.get_proc(), remove_ref_action(), sender_manager);
  }
  struct add_remove_ref_action:
    public action_impl<add_remove_ref_action,
                       wrap<decltype(add_remove_ref), add_remove_ref>>
  {
  };
  
  global_manager_t::~global_manager_t()
  {
    assert(refs == 0);
    if (owner) {
      apply(owner.get_proc(), remove_ref_action(), owner);
    }
    deleter();
  }
  
  
  
  template<typename T>
  template<class Archive>
  void global_shared_ptr<T>::save(Archive& ar, unsigned int version) const
  {
    assert(invariant());
    global_ptr<global_manager_t> sender_manager = manager;
    global_ptr<global_manager_t> owner = global_manager_t::get_owner(manager);
    global_manager_t::add_ref(manager);
    ar << ptr << sender_manager << owner;
  }
  
  template<typename T>
  template<class Archive>
  void global_shared_ptr<T>::load(Archive& ar, unsigned int version)
  {
    assert(!manager);
    global_ptr<global_manager_t> sender_manager, sender_owner;
    ar >> ptr >> sender_manager >> sender_owner;
    // If there is a manager object, then there has to be an owner
    const bool has_owner = bool(sender_manager);
    // If the sender's owner is null, then (by definition) sender's
    // manager is the owner
    const bool sender_is_owner = has_owner && !sender_owner;
    const global_ptr<global_manager_t> owner =
      sender_is_owner ? sender_manager : sender_owner;
    if (has_owner) {
      // TODO: keep one manager per object per process, using a
      // database to look up the existing manager (if any)
      if (owner.is_local()) {
        manager = owner.get();
        global_manager_t::add_ref(manager);
        apply(sender_manager.get_proc(), remove_ref_action(), sender_manager);
      } else {
        manager = new global_manager_t(owner, []{});
        if (!sender_is_owner) {
          // We need to register ourselves with the owner, and need to
          // de-register the message with the sender (in this order).
          // We don't need to do anything if the sender is the owner.
          apply(sender_owner.get_proc(),
                add_remove_ref_action(), sender_owner, sender_manager);
        }
      }
    }
    assert(invariant());
  }
  
}

BOOST_CLASS_EXPORT(rpc::remove_ref_action::evaluate);
BOOST_CLASS_EXPORT(rpc::remove_ref_action::finish);
BOOST_CLASS_EXPORT(rpc::add_remove_ref_action::evaluate);
BOOST_CLASS_EXPORT(rpc::add_remove_ref_action::finish);

#endif  // #ifndef RPC_GLOBAL_SHARED_PTR_HH
