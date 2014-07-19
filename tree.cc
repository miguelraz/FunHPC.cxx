#include "rpc.hh"

#include "cxx_either.hh"
#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_maybe.hh"
#include "cxx_monad.hh"
#include "cxx_tree.hh"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Define a function with only one template argument
template <typename T> using function_ = std::function<T(int)>;

// Define a set with only one template argument
template <typename T> using set_ = std::set<T>;

// Define a vector with only one template argument
template <typename T> using vector_ = std::vector<T>;

// Define an either with only one template argument
template <typename T> using either_ = cxx::either<std::string, T>;

// Define a tree with only one template argument
template <typename T> using tree_ = cxx::tree<T, vector_, std::shared_ptr>;

int add_int(int x, int y) { return x + y; }
RPC_ACTION(add_int);

double add_int_double(int x, int y) { return double(x + y); }
RPC_ACTION(add_int_double);

double make_double(int x) { return double(x); }
RPC_ACTION(make_double);

rpc::client<double> make_client_double(int x) {
  return rpc::make_client<double>(x);
}
RPC_ACTION(make_client_double);

int rpc_main(int argc, char **argv) {

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<function_>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<function_, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::monad::bind<function_, double>(u, [](int x) {
          return std::function<double(int)>([](int x) { return double(x); });
        });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto j __attribute__((__unused__)) = cxx::monad::join<function_>(
        cxx::monad::unit<function_>(cxx::monad::unit<function_>(1)));
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<set_>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<set_, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<set_, double>(
        u, [](int x) { return set_<double>{ double(x) }; });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto j __attribute__((__unused__)) = cxx::monad::join<set_>(
        cxx::monad::unit<set_>(cxx::monad::unit<set_>(1)));
    auto z __attribute__((__unused__)) = cxx::monad::zero<set_, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<set_>(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<std::shared_ptr>(1);
    auto m __attribute__((__unused__)) =
        cxx::monad::make<std::shared_ptr, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::monad::bind<std::shared_ptr, double>(
            u, [](int x) { return std::make_shared<double>(x); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto j __attribute__((__unused__)) =
        cxx::monad::join<std::shared_ptr>(cxx::monad::unit<std::shared_ptr>(
            cxx::monad::unit<std::shared_ptr>(1)));
    auto z __attribute__((__unused__)) =
        cxx::monad::zero<std::shared_ptr, int>();
    auto p __attribute__((__unused__)) =
        cxx::monad::plus<std::shared_ptr>(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<rpc::client>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<rpc::client, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<rpc::client, double>(
        u, [](int x) { return rpc::make_client<double>(x); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto j __attribute__((__unused__)) = cxx::monad::join<rpc::client>(
        cxx::monad::unit<rpc::client>(cxx::monad::unit<rpc::client>(1)));
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<rpc::client>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<rpc::client, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::monad::bind<rpc::client, double>(u, make_client_double_action());
    auto f __attribute__((__unused__)) = cxx::fmap(make_double_action(), u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap(add_int_double_action(), u, 1);
    auto j __attribute__((__unused__)) = cxx::monad::join<rpc::client>(
        cxx::monad::unit<rpc::client>(cxx::monad::unit<rpc::client>(1)));
    auto s __attribute__((__unused__)) = cxx::foldl(add_int_action(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) =
        cxx::monad::unit<rpc::shared_future>(1);
    auto m __attribute__((__unused__)) =
        cxx::monad::make<rpc::shared_future, int>(1);
    auto b __attribute__((__unused__)) =
        cxx::monad::bind<rpc::shared_future, double>(
            u, [](int x) { return rpc::make_ready_future<double>(x).share(); });
    auto f __attribute__((__unused__)) =
        cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto j0 __attribute__((__unused__)) = cxx::monad::unit<rpc::shared_future>(
        cxx::monad::unit<rpc::shared_future>(1)).unwrap();
    auto j __attribute__((__unused__)) = cxx::monad::join<rpc::shared_future>(
        cxx::monad::unit<rpc::shared_future>(
            cxx::monad::unit<rpc::shared_future>(1)));
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<vector_>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<vector_, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<vector_, double>(
        u, [](int x) { return vector_<double>(1, x); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto f2 __attribute__((__unused__)) =
        cxx::fmap([](int x, int y) { return double(x + y); }, u, 1);
    auto z __attribute__((__unused__)) = cxx::monad::zero<vector_, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<vector_>(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<either_>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<either_, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<either_, double>(
        u, [](int x) { return either_<double>(x); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto z __attribute__((__unused__)) = cxx::monad::zero<either_, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<either_>(m, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<cxx::maybe>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<cxx::maybe, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<cxx::maybe, double>(
        u, [](int x) { return cxx::maybe<double>(x); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto z __attribute__((__unused__)) = cxx::monad::zero<cxx::maybe, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<cxx::maybe>(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  {
    cxx::tree<double, vector_, std::shared_ptr> t;
    bool e __attribute__((__unused__)) = t.empty();
    size_t s __attribute__((__unused__)) = t.size();
    bool es __attribute__((__unused__)) = t.empty_slow();
    size_t ss __attribute__((__unused__)) = t.size_slow();

    auto r __attribute__((__unused__)) =
        t.foldl([](double s, double v) { return s + v; }, 0.0);

    auto ti __attribute__((__unused__)) =
        cxx::tree<int, vector_, std::shared_ptr>(
            cxx::tree<int, vector_, std::shared_ptr>::fmap(),
            [](double x) { return int(lrint(x)); }, t);
  }

  {
    auto u __attribute__((__unused__)) = cxx::monad::unit<tree_>(1);
    auto m __attribute__((__unused__)) = cxx::monad::make<tree_, int>(1);
    auto b __attribute__((__unused__)) = cxx::monad::bind<tree_, double>(
        u, [](int x) { return cxx::monad::unit<tree_>(double(x)); });
    auto f = cxx::fmap([](int x) { return double(x); }, u);
    auto j __attribute__((__unused__)) = cxx::monad::join<tree_>(
        cxx::monad::unit<tree_>(cxx::monad::unit<tree_>(1)));
    auto z __attribute__((__unused__)) = cxx::monad::zero<tree_, int>();
    auto p __attribute__((__unused__)) = cxx::monad::plus<tree_>(z, u);
    auto s __attribute__((__unused__)) = cxx::foldl(std::plus<int>(), 0, u);
  }

  return 0;
}
