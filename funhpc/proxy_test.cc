#include <funhpc/proxy.hpp>

#include <cereal/access.hpp>
#include <gtest/gtest.h>

using namespace funhpc;

namespace {
struct s {
  template <typename Archive> void serialize(Archive &ar) { ar(x); }
  int x;
};
s make_s() { return {int(rank())}; }

int f(int x) { return x; }
}

TEST(funhpc_proxy, empty) {
  auto pi = proxy<int>();
  auto ps = proxy<s>();
  auto pf = proxy<int (*)(int)>();
  auto pp = proxy<proxy<int>>();
  EXPECT_FALSE(bool(pi));
  EXPECT_FALSE(bool(ps));
  EXPECT_FALSE(bool(pf));
  EXPECT_FALSE(bool(pp));
}

TEST(funhpc_proxy, make_local) {
  auto pi = make_local_proxy<int>(1);
  auto ps = make_local_proxy<s>(s());
  auto pf = make_local_proxy<int (*)(int)>(f);
  auto pp = make_local_proxy<proxy<int>>(make_local_proxy<int>(1));
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(ps));
  EXPECT_TRUE(bool(pf));
  EXPECT_TRUE(bool(pp));
  EXPECT_TRUE(pi.local());
  EXPECT_TRUE(ps.local());
  EXPECT_TRUE(pf.local());
  EXPECT_TRUE(pp.local());
  EXPECT_EQ(1, *pi);
  EXPECT_EQ(1, **pp);
}

TEST(funhpc_proxy, make_remote) {
  auto p = 1 % size();
  auto pi = make_remote_proxy<int>(p, 1);
  auto ps = make_remote_proxy<s>(p, s());
  auto pf = make_remote_proxy<int (*)(int)>(p, f);
  auto pp = make_remote_proxy<proxy<int>>(make_remote_proxy<int>(1));
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(ps));
  EXPECT_TRUE(bool(pf));
  EXPECT_TRUE(bool(pp));
  if (p != rank()) {
    EXPECT_FALSE(pi.local());
    EXPECT_FALSE(ps.local());
    EXPECT_FALSE(pf.local());
    EXPECT_FALSE(pp.local());
  }
  // TODO auto pil = pi.make_local();
  // TODO auto psl = ps.make_local();
  // TODO auto pfl = pf.make_local();
  // TODO auto ppl = pp.make_local();
  // TODO EXPECT_TRUE(bool(pil));
  // TODO EXPECT_TRUE(bool(psl));
  // TODO EXPECT_TRUE(bool(pfl));
  // TODO EXPECT_TRUE(bool(ppl));
  // TODO EXPECT_TRUE(pil.local());
  // TODO EXPECT_TRUE(psl.local());
  // TODO EXPECT_TRUE(pfl.local());
  // TODO EXPECT_TRUE(ppl.local());
  // TODO EXPECT_EQ(1,*pil)
  // TODO auto ppll = ppl.make_local();
  // TODO EXPECT_TRUE(bool(ppll));
  // TODO EXPECT_TRUE(ppll.local());
  // TODO EXPECT_EQ(1,**ppll)
}

TEST(funhpc_proxy, remote) {
  auto p = 1 % size();
  auto pi = remote(p, f, 1);
  auto ps = remote(p, make_s);
  EXPECT_TRUE(bool(pi));
  EXPECT_TRUE(bool(ps));
  if (p != rank()) {
    EXPECT_FALSE(pi.local());
    EXPECT_FALSE(ps.local());
  }
  // TODO auto pil = pi.make_local();
  // TODO auto psl = ps.make_local();
  // TODO EXPECT_TRUE(bool(pil));
  // TODO EXPECT_TRUE(bool(psl));
  // TODO EXPECT_TRUE(pil.local());
  // TODO EXPECT_TRUE(psl.local());
  // TODO EXPECT_EQ(1, *pi);
  // TODO EXPECT_EQ(p, ps->x);
}
