#include <cxx/invoke.hpp>

#include <gtest/gtest.h>

#include <functional>

using namespace cxx;

namespace {

int f0(int x) { return x; }

struct s {
  int d0;
  const int d1;
  s() : d0(1), d1(2) {}
  int m0(int x) { return x; }
  int m1(int x) const { return x; }
};

struct o0 {
  int operator()(int x) { return x; }
};
struct o1 {
  int operator()(int x) const { return x; }
};
struct o0nc {
  o0nc(const o0nc &) = delete;
  o0nc &operator=(const o0nc &) = delete;
  int operator()(int x) { return x; }
};
struct o1nc {
  o1nc(const o1nc &) = delete;
  o1nc &operator=(const o1nc &) = delete;
  int operator()(int x) const { return x; }
};
struct o0nm {
  o0nm(o0nm &&) = delete;
  o0nm &operator=(o0nm &&) = delete;
  int operator()(int x) { return x; }
};
struct o1nm {
  o1nm(o1nm &&) = delete;
  o1nm &operator=(o1nm &&) = delete;
  int operator()(int x) const { return x; }
};

int ffo1c(const o1 &fo1) { return fo1(1); }
int ffo0m(o0 &&fo0) { return std::move(fo0)(1); }
int ffo1m(o1 &&fo1) { return std::move(fo1)(1); }
int ffo0nc(o0nc &&fo0nc) { return std::move(fo0nc)(1); }
int ffo1nc(o1nc &&fo1nc) { return std::move(fo1nc)(1); }
int ffo1nm(const o1nm &fo1nm) { return fo1nm(1); }
}

TEST(cxx_invoke, invoke) {
  EXPECT_EQ(1, invoke(f0, 1));
  EXPECT_EQ(1, invoke(&f0, 1));

  s s0{};
  const s s1{};
  EXPECT_EQ(1, invoke(&s::m0, s0, 1));
  EXPECT_EQ(1, invoke(&s::m1, s0, 1));
  EXPECT_EQ(1, invoke(&s::m1, s1, 1));
  EXPECT_EQ(1, invoke(&s::m0, &s0, 1));
  EXPECT_EQ(1, invoke(&s::m1, &s0, 1));
  EXPECT_EQ(1, invoke(&s::m1, &s1, 1));

  EXPECT_EQ(1, invoke(&s::d0, s0));
  EXPECT_EQ(2, invoke(&s::d1, s0));
  EXPECT_EQ(1, invoke(&s::d0, s1));
  EXPECT_EQ(2, invoke(&s::d1, s1));
  EXPECT_EQ(3, invoke(&s::d0, s0) = 3);

  o0 fo0{};
  o1 fo1{};
  o0nc fo0nc{};
  o1nc fo1nc{};
  o0nm fo0nm{};
  o1nm fo1nm{};
  EXPECT_EQ(1, invoke(fo0, 1));
  EXPECT_EQ(1, invoke(fo1, 1));
  EXPECT_EQ(1, invoke(fo0nc, 1));
  EXPECT_EQ(1, invoke(fo1nc, 1));
  EXPECT_EQ(1, invoke(fo0nm, 1));
  EXPECT_EQ(1, invoke(fo1nm, 1));

  EXPECT_EQ(1, invoke(std::bind(f0, std::placeholders::_1), 1));
  EXPECT_EQ(1, invoke(std::bind(f0, 1)));

  ffo1c(o1{});
  ffo0m(o0{});
  ffo1m(o1{});
  ffo0nc(o0nc{});
  ffo1nc(o1nc{});
  ffo1nm(o1nm{});
  EXPECT_EQ(1, invoke(std::bind(fo0, 1)));
  EXPECT_EQ(1, invoke(std::bind(fo1, 1)));
  // std::bind(fo0nm, 1);
  // std::bind(fo1nm, 1);
  // std::bind(fo0nc, 1);
  // std::bind(fo1nc, 1);
  // EXPECT_EQ(1,invoke(std::bind(fo0nm, 1)));
  // EXPECT_EQ(1,invoke(std::bind(fo1nm, 1)));
  // EXPECT_EQ(1,invoke(std::bind(fo0nc, 1)));
  // EXPECT_EQ(1,invoke(std::bind(fo1nc, 1)));

  std::bind(ffo1c, o1());
  std::bind(ffo0m, o0());
  std::bind(ffo1m, o1());
  EXPECT_EQ(1, std::bind(ffo1c, o1())());
  // EXPECT_EQ(1,std::bind(ffo0m, o0())());
  // EXPECT_EQ(1,std::bind(ffo1m, o1())());

  auto rl = invoke([](int x) { return x; }, 1);
  EXPECT_EQ(1, rl);
  auto rml = invoke([](int x) mutable { return x; }, 1);
  EXPECT_EQ(1, rml);
}

TEST(cxx_invoke, invoke_of_t) {
  typedef invoke_of_t<int (*)(int), int> a0;
  typedef invoke_of_t<int (*)(int), const int &> a1;
  typedef invoke_of_t<int (*)(int), int &> a2;
  typedef invoke_of_t<int (*)(int), int &&> a3;
  EXPECT_TRUE((std::is_same<a0, int>::value));
  EXPECT_TRUE((std::is_same<a1, int>::value));
  EXPECT_TRUE((std::is_same<a2, int>::value));
  EXPECT_TRUE((std::is_same<a3, int>::value));

  typedef invoke_of_t<int (&)(int), int> b0;
  typedef invoke_of_t<int (&)(int), const int &> b1;
  typedef invoke_of_t<int (&)(int), int &> b2;
  typedef invoke_of_t<int (&)(int), int &&> b3;
  EXPECT_TRUE((std::is_same<b0, int>::value));
  EXPECT_TRUE((std::is_same<b1, int>::value));
  EXPECT_TRUE((std::is_same<b2, int>::value));
  EXPECT_TRUE((std::is_same<b3, int>::value));

  auto c0 = [](int) {};
  typedef invoke_of_t<decltype(c0), int> c1;
  EXPECT_TRUE((std::is_same<c1, void>::value));
}
