#include <hpx/hpx_init.hpp>

namespace rpc {
  int real_main(int argc, char** argv);
}

int hpx_main(int argc, char** argv)
{
  return rpc::real_main(argc, argv);
}

namespace hpx {
  
  int thread_main(int argc, char** argv)
  {
    int iret = hpx::init(argc, argv);
    hpx::finalize();
    return iret;
  }
  
  void thread_initialize()
  {
  }
  
  void thread_finalize()
  {
  }
  
}
