#ifndef HPX_HH
#define HPX_HH

// TODO: include only the headers that are actually needed
// #include <hpx/hpx.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/future.hpp>
#include <hpx/include/threads.hpp>
#include <hpx/version.hpp>

#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>

#include <chrono>
#include <utility>
#include <type_traits>

namespace hpx {
  int thread_main(int argc, char** argv);
  void thread_initialize();
  void thread_finalize();
}

namespace rpc {
  
  using ::boost::lock_guard;
  using ::boost::mutex;
  
  using ::hpx::async;
  using ::hpx::launch;
  using ::hpx::make_ready_future;
  using ::hpx::promise;
  using ::hpx::thread;
  
  using ::hpx::future;
  using ::hpx::shared_future;
  
  namespace this_thread {
    using ::hpx::this_thread::yield;
    using ::hpx::this_thread::get_id;
    
    // get_worker_id
    inline std::size_t get_worker_id()
    {
      return ::hpx::get_worker_thread_num();
    }
    
    // sleep_for
    template<typename Rep, typename Period>
    inline void sleep_for(std::chrono::duration<Rep, Period> const& p)
    {
      typedef boost::ratio<Period::num, Period::den> PeriodBoost;
      auto pBoost = boost::chrono::duration<Rep, PeriodBoost>(p.count());
      ::hpx::this_thread::sleep_for(pBoost);
    }
  }
  
  using ::hpx::thread_main;
  using ::hpx::thread_initialize;
  using ::hpx::thread_finalize;
  
}

namespace std {
  
  // Poison std:: functionality that is also provided by HPX
  struct hpx_incomplete;
  typedef hpx_incomplete async;
  typedef hpx_incomplete future;
  typedef hpx_incomplete lock_guard;
  typedef hpx_incomplete mutex;
  typedef hpx_incomplete promise;
  typedef hpx_incomplete shared_future;
  typedef hpx_incomplete this_thread;
  typedef hpx_incomplete thread;
  
}

#define HPX_HH_DONE
#else
#  ifndef HPX_HH_DONE
#    error "Cyclic include dependency"
#  endif
#endif  // HPX_HH