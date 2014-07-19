#include "rpc.hh"

#include "cxx_foldable.hh"
#include "cxx_functor.hh"
#include "cxx_kinds.hh"
#include "cxx_monad.hh"
#include "cxx_tree.hh"

#include "hwloc.hh"

#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

using rpc::async;
using rpc::broadcast;
using rpc::client;
using rpc::find_all_processes;
using rpc::future;
using rpc::launch;
using rpc::make_client;
using rpc::make_ready_future;
using rpc::make_remote_client;
using rpc::remote;
using rpc::server;
using rpc::shared_future;
using rpc::sync;
using rpc::thread;

using std::cerr;
using std::cout;
using std::flush;
using std::ofstream;
using std::ios_base;
using std::iota;
using std::make_shared;
using std::max;
using std::min;
using std::move;
using std::numeric_limits;
using std::ostream;
using std::ostringstream;
using std::ptrdiff_t;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::vector;

////////////////////////////////////////////////////////////////////////////////

// Integer division, rounding down
template <typename T> T div_floor(T x, T y) {
  assert(y > 0);
  T r = (x >= 0 ? x : x - y + 1) / y;
  assert(r * y <= x && (r + 1) * y > x);
  return r;
}

// Integer modulo, rounding down
template <typename T> T mod_floor(T x, T y) {
  T r = x - div_floor(x, y) * y;
  assert(r >= 0 && r < y);
  return r;
}

// Integer division, rounding up
template <typename T> T div_ceil(T x, T y) {
  assert(y > 0);
  T r = (x > 0 ? x + y - 1 : x) / y;
  assert(r * y >= x && (r - 1) * y < x);
  return r;
}

// Integer modulo, rounding up
template <typename T> T mod_ceil(T x, T y) {
  T r = x - div_ceil(x, y) * y;
  assert(r >= 0 && r < y);
  return r;
}

////////////////////////////////////////////////////////////////////////////////

// An empty serializable type as mix-in
using empty = tuple<>;

////////////////////////////////////////////////////////////////////////////////

// Global definitions, a poor man's parameter file

struct defs_t {
  // const ptrdiff_t rho = 1; // resolution scale
  const ptrdiff_t rho = 10; // resolution scale
  const ptrdiff_t ncells_per_grid = 10;

  const double xmin = 0.0;
  const double xmax = 1.0;
  const double cfl = 0.5;
  const double tmin = 0.0;
  const double tmax = 1.0;
  // const ptrdiff_t nsteps = -1;
  const ptrdiff_t nsteps = 10;

  ptrdiff_t ncells;
  double dx;
  double dt;

  const ptrdiff_t wait_every = 0;
  // const ptrdiff_t info_every = 10;
  const ptrdiff_t info_every = 0;
  // const ptrdiff_t file_every = 0;
  const ptrdiff_t file_every = -1;
  defs_t(int nprocs, int nthreads)
      : ncells(rho * ncells_per_grid * nprocs * nthreads),
        dx((xmax - xmin) / ncells), dt(cfl * dx) {}

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(ncells, dx, dt);
  }

public:
  defs_t() {} // only for serialize
};
shared_ptr<defs_t> defs;
void set_defs(shared_ptr<defs_t> defs) { ::defs = defs; }
RPC_ACTION(set_defs);
void set_all_defs(const shared_ptr<defs_t> &defs) {
  async_broadcast(set_defs_action(), defs).get();
}

////////////////////////////////////////////////////////////////////////////////

// Linear combinations

// TODO: introduce the notion of a vector space for this?

// inline double axpy(double a, double x, double y) { return a * x + y; }
// RPC_ACTION(axpy);

////////////////////////////////////////////////////////////////////////////////

// A norm

// TODO: introduce cxx_monoid for this?
// mempty, mappend, mconcat
// also: sum, product, any, all

struct norm_t {
  double sum, sum2, count;

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(sum, sum2, count);
  }

public:
  norm_t() : sum(0.0), sum2(0.0), count(0.0) {}
  norm_t(double val) : sum(val), sum2(pow(val, 2)), count(1.0) {}
  norm_t(const norm_t &x, const norm_t &y)
      : sum(x.sum + y.sum), sum2(x.sum2 + y.sum2), count(x.count + y.count) {}
  double avg() const { return sum / count; }
  double norm2() const { return sqrt(sum2 / count); }
};
RPC_COMPONENT(norm_t);
inline norm_t operator+(const norm_t &x, const norm_t &y) {
  return norm_t(x, y);
}

////////////////////////////////////////////////////////////////////////////////

// A cell -- a container holding data, no intelligence here

struct cell_t {
  static constexpr double nan = numeric_limits<double>::quiet_NaN();

  double x;
  double u;
  double rho;
  double v;

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) { ar(x, u, rho, v); }

public:
  // For safety
  cell_t() : x(nan), u(nan), rho(nan), v(nan) {}

  // Output
  ostream &output(ostream &os) const {
    RPC_ASSERT(server->rank() == 0);
    return os << "cell: x=" << x << " u=" << u << " rho=" << rho << " v=" << v;
  }

  // Linear combination
  struct axpy : empty {};
  cell_t(axpy, double a, const cell_t &x, const cell_t &y) {
    this->x = a * x.x + y.x;
    u = a * x.u + y.u;
    rho = a * x.rho + y.rho;
    v = a * x.v + y.v;
  }

  // Norm
  norm_t norm() const { return norm_t(u) + norm_t(rho) + norm_t(v); }

  // Analytic solution
  struct analytic : empty {};
  cell_t(analytic, double t, double x) {
    this->x = x;
    u = sin(2 * M_PI * t) * sin(2 * M_PI * x);
    rho = 2 * M_PI * cos(2 * M_PI * t) * sin(2 * M_PI * x);
    v = 2 * M_PI * sin(2 * M_PI * t) * cos(2 * M_PI * x);
  }

  // Initial condition
  struct initial : empty {};
  cell_t(initial, double t, double x) : cell_t(analytic(), t, x) {}

  // Boundary condition
  struct boundary : empty {};
  cell_t(boundary, double t, double x) : cell_t(analytic(), t, x) {}
  RPC_DECLARE_CONSTRUCTOR(cell_t, boundary, double, double);

  // Error
  struct error : empty {};
  cell_t(error, const cell_t &c, double t)
      : cell_t(axpy(), -1.0, cell_t(analytic(), t, c.x), c) {}

  // RHS
  struct rhs : empty {};
  cell_t(rhs, const cell_t &c, const cell_t &cm, const cell_t &cp) {
    x = 0.0; // dx/dt
    u = c.rho;
    rho = (cp.v - cm.v) / (2 * defs->dx);
    v = (cp.rho - cm.rho) / (2 * defs->dx);
  }
};
RPC_COMPONENT(cell_t);
RPC_IMPLEMENT_CONSTRUCTOR(cell_t, cell_t::boundary, double, double);

// Output
ostream &operator<<(ostream &os, const cell_t &c) { return c.output(os); }

////////////////////////////////////////////////////////////////////////////////

// Each grid lives on a process

// TODO: use array<...,N>, and allow multiple ghost zones
template <template <typename> class M, typename T, typename F>
M<typename cxx::invoke_of<F, T, T, T>::type>
stencil_fmap(const F &f, const M<T> &c, const T &bm, const T &bp) {
  typedef typename cxx::invoke_of<F, T, T, T>::type R;
  size_t s = c.size();
  assert(s >= 2); // TODO: This can be avoided
  M<R> r(s);
  // TODO: add push_back equivalent to monad
  // TODO: for efficiency: don't use push_back, use something else
  // that requires preallocation, that doesn't reallocate, and which
  // turns into an indexed loop when used for vectors
  r[0] = cxx::invoke(f, c[0], bm, c[1]);
  for (size_t i = 1; i < s - 1; ++i)
    r[i] = cxx::invoke(f, c[i], c[i - 1], c[i + 1]);
  r[s - 1] = cxx::invoke(f, c[s - 1], c[s - 2], bp);
  return r;
}

template <typename CT,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::type>
C<T> unit(const T &x) {
  return cxx::monad::unit<C>(x);
}
template <typename CT,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::type>
C<T> unit(T &&x) {
  return cxx::monad::unit<C>(std::move(x));
}

template <typename CT, typename... As,
          template <typename> class C = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::type>
C<T> make(As &&... as) {
  return cxx::monad::make<C, T>(std::forward<As>(as)...);
}

template <typename CCT,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::type>
typename std::enable_if<std::is_same<C2<T>, C<T> >::value, C<T> >::type
join(const CCT &xss) {
  return cxx::monad::join<C, T>(xss);
}
template <typename CCT,
          template <typename> class C = cxx::kinds<CCT>::template constructor,
          typename CT = typename cxx::kinds<CCT>::type,
          template <typename> class C2 = cxx::kinds<CT>::template constructor,
          typename T = typename cxx::kinds<CT>::type>
typename std::enable_if<std::is_same<C2<T>, C<T> >::value, C<T> >::type
join(CCT &&xss) {
  return cxx::monad::join<C, T>(std::move(xss));
}

template <typename MT, typename B, typename F, typename G,
          template <typename> class M = cxx::kinds<MT>::template constructor,
          typename T = typename cxx::kinds<MT>::type,
          typename R = typename cxx::invoke_of<F, T, B, B>::type>
typename std::enable_if<
    std::is_same<typename cxx::invoke_of<G, T, bool>::type, B>::value,
    M<R> >::type
stencil_fmap(const F &f, const G &g, const MT &c, const B &bm, const B &bp) {
  size_t s = c.size();
  assert(s >= 2); // TODO: This can be avoided
  M<R> r(s);
  // TODO: add push_back equivalent to constructor
  // TODO: for efficiency: don't use push_back, use something else
  // that requires preallocation, that doesn't reallocate, and which
  // turns into an indexed loop when used for vectors
  r[0] = cxx::invoke(f, c[0], bm, cxx::invoke(g, c[1], false));
  for (size_t i = 1; i < s - 1; ++i)
    r[i] = cxx::invoke(f, c[i], cxx::invoke(g, c[i - 1], true),
                       cxx::invoke(g, c[i + 1], false));
  r[s - 1] = cxx::invoke(f, c[s - 1], cxx::invoke(g, c[s - 2], true), bp);
  return r;
}

struct grid_t {
  // TODO: introduce irange for these two (e.g. irange_t =
  // array<ptrdiff_t,2>)
  ptrdiff_t imin, imax; // spatial indices
  static double x(ptrdiff_t i) { return defs->xmin + (i + 0.5) * defs->dx; }

  template <typename T> using vector_ = vector<T>;
  vector<cell_t> cells;

private:
  friend class cereal::access;
  template <typename Archive> void serialize(Archive &ar) {
    ar(imin, imax, cells);
  }
  // template <typename Archive>
  // static void load_and_construct(Archive &ar,
  //                                cereal::construct<grid_t> &construct) {
  //   ptrdiff_t imin, imax;
  //   vector<cell_t> cells;
  //   ar(imin, imax, cells);
  //   construct(imin, imax, std::move(cells));
  // }

public:
  grid_t() {} // only for serialization
  // grid_t(ptrdiff_t imin, ptrdiff_t imax, const vector<cell_t> &cells)
  //     : imin(imin), imax(imax), cells(cells) {}
  // grid_t(ptrdiff_t imin, ptrdiff_t imax, vector<cell_t> &&cells)
  //     : imin(imin), imax(imax), cells(std::move(cells)) {}

  const cell_t &get(ptrdiff_t i) const {
    assert(i >= imin && i < imax);
    return cells[i - imin];
  }

  cell_t get_boundary(bool face_upper) const {
    return get(face_upper ? imax - 1 : imin);
  }

  // Output
  // TODO: create output constructor (based on tree)
  ostream &output(ostream &os) const {
    RPC_ASSERT(server->rank() == 0);
    for (ptrdiff_t i = imin; i < imax; ++i) {
      os << "   i=" << i << " " << get(i) << "\n";
    }
    return os;
  }

  // Linear combination
  struct axpy : empty {};
  grid_t(axpy, double a, const grid_t &x, const grid_t &y)
      : imin(y.imin), imax(y.imax),
        cells(cxx::fmap([a](const cell_t &x, const cell_t &y) {
                          return cell_t(cell_t::axpy(), a, x, y);
                        },
                        x.cells, y.cells)) {}

  // Norm
  norm_t norm() const {
    return cxx::foldl([](const norm_t &x,
                         const cell_t &y) { return x + y.norm(); },
                      norm_t(), cells);
  }

  // Initial condition
  struct initial : empty {};
  grid_t(initial, double t, ptrdiff_t imin, ptrdiff_t imax)
      : imin(imin), imax(imax),
        cells(
            cxx::fmap([t](int i) { return cell_t(cell_t::initial(), t, x(i)); },
                      [imin, imax]() {
                        vector<ptrdiff_t> is(imax - imin);
                        iota(is.begin(), is.end(), imin);
                        return is;
                      }())) {}

  // Error
  struct error : empty {};
  grid_t(error, const grid_t &g, double t)
      : imin(g.imin), imax(g.imax),
        cells(cxx::fmap([t](const cell_t &c) {
                          return cell_t(cell_t::error(), c, t);
                        },
                        g.cells)) {}

  // RHS
  struct rhs : empty {};
  grid_t(rhs, const grid_t &g, const cell_t &bm, const cell_t &bp)
      : imin(g.imin), imax(g.imax),
        cells(stencil_fmap<vector_>([](const cell_t &c, const cell_t &cm,
                                       const cell_t &cp) {
                                      return cell_t(cell_t::rhs(), c, cm, cp);
                                    },
                                    g.cells, bm, bp)) {}
};
RPC_COMPONENT(grid_t);

// Output
ostream &operator<<(ostream &os, const client<grid_t> &g) {
  return g->output(os);
}

cell_t grid_get_boundary(const grid_t &g, bool face_upper) {
  return g.get_boundary(face_upper);
}
RPC_ACTION(grid_get_boundary);
typedef cxx::detail::fmap_action<grid_t, grid_get_boundary_action,
                                 bool>::evaluate
grid_get_boundary_action_evaluate;
typedef cxx::detail::fmap_action<grid_t, grid_get_boundary_action, bool>::finish
grid_get_boundary_action_finish;
RPC_CLASS_EXPORT(grid_get_boundary_action_evaluate);
RPC_CLASS_EXPORT(grid_get_boundary_action_finish);

// Note: Arguments re-ordered
grid_t grid_axpy(const grid_t &y, double a, const grid_t &x) {
  return grid_t(grid_t::axpy(), a, x, y);
}
RPC_ACTION(grid_axpy);
typedef cxx::detail::fmap_action<grid_t, grid_axpy_action, double,
                                 rpc::client<grid_t> >::evaluate
grid_axpy_action_evaluate;
typedef cxx::detail::fmap_action<grid_t, grid_axpy_action, double,
                                 rpc::client<grid_t> >::finish
grid_axpy_action_finish;
RPC_CLASS_EXPORT(grid_axpy_action_evaluate);
RPC_CLASS_EXPORT(grid_axpy_action_finish);

norm_t grid_norm_foldl(const norm_t &x, const grid_t &y) {
  return x + y.norm();
}
RPC_ACTION(grid_norm_foldl);
typedef cxx::client::foldl_action<
    norm_t, grid_t, grid_norm_foldl_action>::evaluate grid_norm_foldl_evaluate;
typedef cxx::client::foldl_action<
    norm_t, grid_t, grid_norm_foldl_action>::finish grid_norm_foldl_finish;
RPC_CLASS_EXPORT(grid_norm_foldl_evaluate);
RPC_CLASS_EXPORT(grid_norm_foldl_finish);

// Note: Arguments re-ordered
grid_t grid_initial(ptrdiff_t imin, ptrdiff_t imax, double t) {
  return grid_t(grid_t::initial(), t, imin, imax);
}
RPC_ACTION(grid_initial);
typedef cxx::detail::fmap_action<ptrdiff_t, grid_initial_action,
                                 rpc::client<ptrdiff_t>,
                                 double>::evaluate grid_initial_action_evaluate;
typedef cxx::detail::fmap_action<ptrdiff_t, grid_initial_action,
                                 rpc::client<ptrdiff_t>,
                                 double>::finish grid_initial_action_finish;
RPC_CLASS_EXPORT(grid_initial_action_evaluate);
RPC_CLASS_EXPORT(grid_initial_action_finish);

grid_t grid_error(const grid_t &g, double t) {
  return grid_t(grid_t::error(), g, t);
}
RPC_ACTION(grid_error);
typedef cxx::detail::fmap_action<grid_t, grid_error_action, double>::evaluate
grid_error_action_evaluate;
typedef cxx::detail::fmap_action<grid_t, grid_error_action, double>::finish
grid_error_action_finish;
RPC_CLASS_EXPORT(grid_error_action_evaluate);
RPC_CLASS_EXPORT(grid_error_action_finish);

grid_t grid_rhs(const grid_t &g, const cell_t &bm, const cell_t &bp) {
  return grid_t(grid_t::rhs(), g, bm, bp);
}
RPC_ACTION(grid_rhs);
typedef cxx::detail::fmap_action<grid_t, grid_rhs_action, rpc::client<cell_t>,
                                 rpc::client<cell_t> >::evaluate
grid_rhs_action_evaluate;
typedef cxx::detail::fmap_action<grid_t, grid_rhs_action, rpc::client<cell_t>,
                                 rpc::client<cell_t> >::finish
grid_rhs_action_finish;
RPC_CLASS_EXPORT(grid_rhs_action_evaluate);
RPC_CLASS_EXPORT(grid_rhs_action_finish);

////////////////////////////////////////////////////////////////////////////////

// The domain is distributed over multiple processes

template <typename T> using vector_ = std::vector<T>;
template <typename T> using tree_ = cxx::tree<T, vector_, shared_ptr>;

struct domain_t {
  // TODO: implement serialize routine

  double t;

  static ptrdiff_t ngrids() {
    return div_ceil(defs->ncells, defs->ncells_per_grid);
  }
  static ptrdiff_t cell2grid(ptrdiff_t icell) {
    assert(icell >= 0 && icell < defs->ncells);
    return div_floor(icell, defs->ncells_per_grid);
  }
  vector<client<grid_t> > grids;

private:
  // Note: The grids are not allocated in the constructor; this has to
  // be done explicitly.
  domain_t(double t) : t(t), grids(ngrids()) {}
  domain_t(const client<domain_t> &d) : t(d->t), grids(ngrids()) {}
  void set(ptrdiff_t i, const client<grid_t> &g) {
    assert(i >= 0 && i < ngrids() && grids.size() == ngrids());
    grids[i] = g;
  }

public:
  // Note: Be careful about this being a reference! This only works
  // because references decay in bind and async.
  const client<grid_t> &get(ptrdiff_t i) const {
    assert(i >= 0 && i < ngrids() && grids.size() == ngrids());
    return grids[i];
  }

  // Wait until all grids are ready
  void wait() const { cxx::fmap(&client<grid_t>::wait, grids); }

  // Output
  ostream &output(ostream &os) const {
    RPC_ASSERT(server->rank() == 0);
    os << "domain: t=" << t << "\n";
    for (ptrdiff_t i = 0; i < ngrids(); ++i) {
      // Serialize output by making local copies
      os << "grid[" << i << "]:\n" << get(i).make_local();
    }
    return os;
  }

  // Linear combination
  struct axpy : empty {};
  domain_t(axpy, double a, const domain_t &x, const domain_t &y)
      : t(a * x.t + y.t),
        grids(cxx::fmap([a](const client<grid_t> &x, const client<grid_t> &y) {
                          return cxx::fmap(grid_axpy_action(),
                                           // TODO: grid_t::axpy_action(),
                                           y, a, x);
                        },
                        x.grids, y.grids)) {}
  domain_t(axpy, double a, const client<domain_t> &x, const client<domain_t> &y)
      : domain_t(axpy(), a, *x, *y) {}

  // Norm
  norm_t norm() const {
    return cxx::foldl([](const norm_t &x, const client<grid_t> &y) {
                        return x + cxx::foldl(grid_norm_foldl_action(),
                                              norm_t(), y);
                      },
                      norm_t(), grids);
  }

  // Initial condition
  struct initial : empty {};
  domain_t(initial, double t)
      : t(t),
        grids(cxx::fmap([t](const client<ptrdiff_t> &imin,
                            const client<ptrdiff_t> &imax) {
                          return cxx::fmap(grid_initial_action(), imin, imax,
                                           t);
                        },
                        []() {
                          // Choose a domain decomposition
                          vector<client<ptrdiff_t> > imins(ngrids());
                          for (ptrdiff_t i = 0; i < ngrids(); ++i)
                            imins[i] = cxx::monad::unit<client>(
                                min(i * defs->ncells_per_grid, defs->ncells));
                          return imins;
                        }(),
                        []() {
                          // Choose a domain decomposition
                          vector<client<ptrdiff_t> > imaxs(ngrids());
                          for (ptrdiff_t i = 0; i < ngrids(); ++i)
                            imaxs[i] = cxx::monad::unit<client>(min(
                                (i + 1) * defs->ncells_per_grid, defs->ncells));
                          return imaxs;
                        }())) {}

  // Error
  struct error : empty {};
  domain_t(error, const domain_t &s)
      : t(s.t),
        grids(cxx::fmap([this /*TODO: t*/](const client<grid_t> &s) {
                          return cxx::fmap(grid_error_action(), s, this->t);
                        },
                        s.grids)) {}
  domain_t(error, const client<domain_t> &s) : domain_t(error(), *s) {}

  // RHS
  struct rhs : empty {};
  domain_t(rhs, const domain_t &s)
      : t(1.0), // dt/dt
        grids(stencil_fmap(
            [](const client<grid_t> &g, const client<cell_t> &bm,
               const client<cell_t> &bp) {
              return cxx::fmap(grid_rhs_action(), g, bm, bp);
            },
            [](const client<grid_t> &g, bool face_upper) {
              return cxx::fmap(grid_get_boundary_action(), g, face_upper);
            },
            s.grids, cxx::monad::make<client, cell_t>(
                         cell_t::boundary(), s.t, defs->xmin - 0.5 * defs->dx),
            cxx::monad::make<client, cell_t>(cell_t::boundary(), s.t,
                                             defs->xmax + 0.5 * defs->dx))) {}
  domain_t(rhs, const client<domain_t> &s) : domain_t(rhs(), *s) {}
};

// Output
ostream &operator<<(ostream &os, const client<domain_t> &d) {
  return d->output(os);
}

////////////////////////////////////////////////////////////////////////////////

// Memoized data for the current iteration

struct memoized_t {
  ptrdiff_t n;
  client<domain_t> state;
  client<domain_t> rhs;
  client<domain_t> error;
  shared_future<norm_t> error_norm;
  memoized_t(ptrdiff_t n, const client<domain_t> &state) : n(n), state(state) {
    rhs = make_client<domain_t>(launch::deferred, domain_t::rhs(), state);
    error = make_client<domain_t>(launch::deferred, domain_t::error(), state);
    error_norm = async(launch::deferred, &domain_t::norm, error);
  }
};

// RK2
client<domain_t> rk2(const shared_ptr<memoized_t> &m) {
  const client<domain_t> &s0 = m->state;
  const client<domain_t> &r0 = m->rhs;
  // Step 1
  auto s1 = make_client<domain_t>(domain_t::axpy(), 0.5 * defs->dt, r0, s0);
  auto r1 = make_client<domain_t>(domain_t::rhs(), s1);
  // Step 2
  return make_client<domain_t>(domain_t::axpy(), defs->dt, r1, s0);
}

// Output
ostream *do_info_output(const shared_future<ostream *> &fos,
                        const shared_ptr<memoized_t> &m) {
  RPC_ASSERT(server->rank() == 0);
  const shared_ptr<domain_t> &s = m->state.get();
  const norm_t &en = m->error_norm.get();
  ostream *os = fos.get();
  *os << "n=" << m->n << " t=" << s->t << " "
      << "L2-norm[error]: " << en.norm2() << "\n" << flush;
  return os;
}

shared_future<ostream *> info_output(shared_future<ostream *> fos,
                                     const shared_ptr<memoized_t> &m) {
  if (defs->info_every >= 0 &&
      ((m->n == 0 || m->n == defs->nsteps) ||
       (defs->info_every > 0 && m->n % defs->info_every == 0))) {
    fos = async(do_info_output, fos, m);
  }
  return fos;
}

ostream *do_file_output(const shared_future<ostream *> fos,
                        const shared_ptr<memoized_t> &m) {
  RPC_ASSERT(server->rank() == 0);
  ostream *os = fos.get();
  *os << "State: " << m->state << "RHS: " << m->rhs
      << "L2-norm[error]: " << m->error_norm.get().norm2() << "\n"
      << "\n" << flush;
  return os;
}

shared_future<ostream *> file_output(shared_future<ostream *> fos,
                                     const shared_ptr<memoized_t> &m) {
  if (defs->file_every >= 0 &&
      ((m->n == 0 || m->n == defs->nsteps) ||
       (defs->file_every > 0 && m->n % defs->file_every == 0))) {
    fos = async(do_file_output, fos, m);
  }
  return fos;
}

////////////////////////////////////////////////////////////////////////////////

// Driver

string get_thread_stats() {
  ostringstream os;
  rpc::thread_stats_t thread_stats = rpc::get_thread_stats();
  os << "[" << rpc::server->rank() << "] Thread statistics:\n"
     << "   threads started: " << thread_stats.threads_started << "\n"
     << "   threads stopped: " << thread_stats.threads_stopped << "\n"
     << "   threads still running: "
     << thread_stats.threads_started - thread_stats.threads_stopped << "\n";
  return os.str();
}
RPC_ACTION(get_thread_stats);

string get_server_stats() {
  ostringstream os;
  rpc::server_base::stats_t server_stats = rpc::server->get_stats();
  os << "[" << rpc::server->rank() << "] Server statistics:\n"
     << "   messages sent: " << server_stats.messages_sent << "\n"
     << "   messages received: " << server_stats.messages_received << "\n";
  return os.str();
}
RPC_ACTION(get_server_stats);

int rpc_main(int argc, char **argv) {
  // cout << "Determining CPU bindings via hwloc:\n";
  // hwloc_bindings(false);
  // cout << "Setting CPU bindings via hwloc:\n";
  // hwloc_bindings(true);

  set_all_defs(
      make_shared<defs_t>(server->size(), thread::hardware_concurrency()));

  shared_future<ostream *> fio = make_ready_future<ostream *>(&cout);
  ostringstream filename;
  filename << "wave." << rpc::server->size() << ".txt";
  ofstream file(filename.str(), ios_base::trunc);
  shared_future<ostream *> ffo = make_ready_future<ostream *>(&file);

  auto t0 = std::chrono::high_resolution_clock::now();

  auto s = make_client<domain_t>(domain_t::initial(), defs->tmin);
  auto m = make_shared<memoized_t>(0, s);
  fio = info_output(fio, m);
  ffo = file_output(ffo, m);

  while ((defs->nsteps < 0 || m->n < defs->nsteps) &&
         (defs->tmax < 0.0 || m->state->t < defs->tmax + 0.5 * defs->dt)) {

    if (defs->wait_every != 0 && m->n % defs->wait_every == 0) {
      // Rate limiter
      s.wait();
      fio.wait();
      ffo.wait();
    }

    s = rk2(m);
    m = make_shared<memoized_t>(m->n + 1, s);
    fio = info_output(fio, m);
    ffo = file_output(ffo, m);
  }

  fio.wait();
  ffo.wait();

  auto t1 = std::chrono::high_resolution_clock::now();
  double elapsed =
      std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count() /
      1.0e+9;

  cout << "Elapsed time: " << elapsed << " sec\n";
  for (int p = 0; p < rpc::server->size(); ++p) {
    cout << sync(remote::sync, p, get_thread_stats_action());
  }
  for (int p = 0; p < rpc::server->size(); ++p) {
    cout << sync(remote::sync, p, get_server_stats_action());
  }

  file.close();

  return 0;
}
