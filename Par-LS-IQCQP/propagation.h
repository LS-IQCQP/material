#pragma once

#include <assert.h>

#include <limits>
#include <list>
#include <memory>
#include <set>

#include "sol.h"
#include "util.h"
namespace solver {
struct var;  // 前向声明
}

namespace solver {
namespace range {
class range {
 public:
  static constexpr Float eps = 1e-6 - 1e-13;
  enum class status { VALID, INVALID, CONSTANT };

  Float eps_floor(Float v) const { return std::floor(v + eps); }
  Float eps_ceil(Float v) const { return std::ceil(v - eps); }

  bool range_valid(Float lower, Float upper) const {
    if (lower != lower || upper != upper) return false;
    if (lower == std::numeric_limits<Float>::infinity() || upper == -std::numeric_limits<Float>::infinity())
      return false;
    if (lower - upper > eps) return false;
    return true;
  }

  virtual ~range() = default;

  virtual std::unique_ptr<range> operator+(const range &r) = 0;
  virtual std::unique_ptr<range> operator-(const range &r) = 0;
  virtual std::unique_ptr<range> operator*(const range &r) = 0;
  virtual std::unique_ptr<range> operator*(Float n) = 0;
  virtual std::unique_ptr<range> square() = 0;

  virtual std::unique_ptr<range> copy() = 0;

  virtual void set(const Float v) = 0;
  virtual void set(const int v) = 0;
  virtual void set(const var &v) = 0;

  virtual bool is_int() const = 0;
  virtual Float lower() const = 0;
  virtual Float upper() const = 0;
  virtual status get_status() = 0;

  virtual bool in_range(const Float v) = 0;
  virtual const std::list<std::pair<Float, Float>> &get_range() const = 0;
  virtual status intersect(range &r) = 0;
};

class fuzzy_range : public range {
 public:
  fuzzy_range() {
    _lower = -std::numeric_limits<Float>::infinity();
    _upper = std::numeric_limits<Float>::infinity();
    _is_int = false;
  }
  fuzzy_range(const Float lower, const Float upper, bool is_int) {
    _lower = is_int ? eps_ceil(lower) : lower;
    _upper = is_int ? eps_floor(upper) : upper;
    _is_int = is_int;
  }
  fuzzy_range(const var &v);
  ~fuzzy_range() = default;

  std::unique_ptr<range> operator+(const range &r) override;
  std::unique_ptr<range> operator-(const range &r) override;
  std::unique_ptr<range> operator*(const range &r) override;
  std::unique_ptr<range> operator*(Float n) override;
  std::unique_ptr<range> square() override;

  std::unique_ptr<range> copy() override { return std::make_unique<fuzzy_range>(_lower, _upper, _is_int); }

  void set(const Float v) override { _lower = _upper = v; }
  void set(const int v) override { _lower = _upper = static_cast<Float>(v); }
  void set(const var &v) override;

  bool is_int() const override { return _is_int; }
  Float lower() const override { return _lower; }
  Float upper() const override { return _upper; }
  status get_status() override;

  bool in_range(Float v) override {
    if (_is_int && !(std::abs(v - std::round(v)) <= eps)) return false;
    return _lower - v <= eps && v - _upper <= eps;
  }
  const std::list<std::pair<Float, Float>> &get_range() const override {
    static thread_local std::list<std::pair<Float, Float>> ranges = {};
    ranges.clear();
    ranges.emplace_back(_lower, _upper);
    return ranges;
  }
  status intersect(range &r) override;

 private:
  bool _is_int;
  Float _lower, _upper;
};

class quad_fuzzy_range : public range {
 public:
  quad_fuzzy_range() {
    _is_int = false;
    _f_ranges.emplace_back(-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity());
  }
  quad_fuzzy_range(const Float lower, const Float upper, const bool is_int) : _is_int(is_int) {
    Float _lower = is_int ? eps_ceil(lower) : lower, _upper = is_int ? eps_floor(upper) : upper;
    if (_lower - _upper <= eps) _f_ranges.emplace_back(_lower, _upper);
  }
  quad_fuzzy_range(const std::list<std::pair<Float, Float>> &ranges, const bool is_int) : _is_int(is_int) {
    for (const auto &p : ranges) {
      std::pair<Float, Float> r = p;
      if (is_int) {
        r.first = eps_ceil(r.first);
        r.second = eps_floor(r.second);
      }
      if (r.first - r.second <= eps) _f_ranges.push_back(r);
    }
  }
  quad_fuzzy_range(const var &v);
  ~quad_fuzzy_range() = default;

  std::unique_ptr<range> operator+(const range &r) override;
  std::unique_ptr<range> operator-(const range &r) override;
  std::unique_ptr<range> operator*(const range &r) override;
  std::unique_ptr<range> operator*(Float n) override;
  std::unique_ptr<range> square() override;

  std::unique_ptr<range> copy() override { return std::make_unique<quad_fuzzy_range>(_f_ranges, _is_int); }

  void set(const Float v) override {
    _f_ranges.clear();
    _f_ranges.emplace_back(v, v);
  }
  void set(const int v) override {
    _f_ranges.clear();
    _f_ranges.emplace_back(static_cast<Float>(v), static_cast<Float>(v));
  }
  void set(const var &v) override;

  bool is_int() const override { return _is_int; }
  Float lower() const override { return _f_ranges.begin()->first; }
  Float upper() const override { return _f_ranges.back().second; }
  status get_status() override;

  bool in_range(Float v) override;
  const std::list<std::pair<Float, Float>> &get_range() const override { return _f_ranges; }
  status intersect(range &r) override;

  void add_range(const range &r);
  void add_range(std::list<std::pair<Float, Float>> &ranges, const std::pair<Float, Float> &inr,
                 std::list<std::pair<Float, Float>>::iterator &it, bool is_int);

 private:
  bool _is_int;
  std::list<std::pair<Float, Float>> _f_ranges;
};

class exact_range : public range {
 public:
  exact_range() {
    _f_ranges.emplace_back(-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity());
  }
  exact_range(const Float lower, const Float upper) { _f_ranges.emplace_back(lower, upper); }
  exact_range(const std::list<std::pair<Float, Float>> &ranges) : _f_ranges(ranges) {}
  exact_range(const var &v);
  ~exact_range() = default;

  /*
   * [TODO] Conduct an experiment to compare the differences between merge sort
   *        and insertion sort (currently implemented using insertion sort).
   *
   * Merge sort:     theoretically has a lower time complexity.
   * Insertion sort: performs better under random conditions
   *                 and is more cache-friendly.
   */
  std::unique_ptr<range> operator+(const range &r) override;
  std::unique_ptr<range> operator-(const range &r) override;
  std::unique_ptr<range> operator*(const range &r) override;
  std::unique_ptr<range> operator*(Float n) override;
  std::unique_ptr<range> square() override;

  std::unique_ptr<range> copy() override { return std::make_unique<exact_range>(_f_ranges); }

  void set(const Float v) override {
    _f_ranges.clear();
    _f_ranges.emplace_back(v, v);
  }
  void set(const int v) override {
    _f_ranges.clear();
    _f_ranges.emplace_back(static_cast<Float>(v), static_cast<Float>(v));
  }
  void set(const var &v) override;

  bool is_int() const override { return false; }
  Float lower() const override { return _f_ranges.begin()->first; }
  Float upper() const override { return _f_ranges.back().second; }
  status get_status() override;

  bool in_range(Float v) override;
  const std::list<std::pair<Float, Float>> &get_range() const override { return _f_ranges; }
  status intersect(range &r) override;

  void add_range(const range &r);
  void add_range(std::list<std::pair<Float, Float>> &ranges, const std::pair<Float, Float> &r,
                 std::list<std::pair<Float, Float>>::iterator &it);

 private:
  std::list<std::pair<Float, Float>> _f_ranges;
};
}  // namespace range

class qp_solver::propagation {
 public:
  enum class range_type { FUZZY, QUAD_FUZZY, EXACT };

  // int gsl_poly_solve_quadratic(Float a, Float b, Float c, Float *root1, Float *root2) {
  //   if (fabs(a) < 1e-6) {  // 处理 a = 0 的情况，变为线性方程 bx + c = 0
  //     return 0;
  //   }
  //   double discriminant = b * b - 4 * a * c;

  //   if (discriminant > 0) {  // 两个不同的实根
  //     double sqrtD = sqrtl(discriminant);
  //     *root1 = (-b - sqrtD) / (2 * a);
  //     *root2 = (-b + sqrtD) / (2 * a);
  //     return 2;
  //   } else if (std::fabs(discriminant) < 0) {  // 重根
  //     *root1 = -b / (2 * a);
  //     return 1;
  //   }
  // }

  propagation() = delete;
  propagation(const propagation &) = delete;
  propagation &operator=(const propagation &) = delete;

  propagation(qp_solver &solver, range_type type = range_type::FUZZY);
  ~propagation() = default;

  Float get_lower(int var_idx) const { return _ranges[var_idx]->lower(); }
  Float get_upper(int var_idx) const { return _ranges[var_idx]->upper(); }
  std::list<std::pair<Float, Float>> get_range(int var_idx) const { return _ranges[var_idx]->get_range(); }
  Float get_reason(int var_idx) const { return _reason_score[var_idx]; }
  bool var_in_range(int var_idx, Float v) const { return _ranges[var_idx]->in_range(v); }
  bool var_is_int(int var_idx) const { return _ranges[var_idx]->is_int(); }
  bool var_is_fixed(int var_idx) const { return _is_fixed[var_idx]; }
  bool var_is_changed(int var_idx) const { return _changed_vars[var_idx]; }

  void init_ranges();
  std::unique_ptr<range::range> gen(Float lower, Float upper);
  std::unique_ptr<range::range> solve_quadratic(Float a, Float b, Float c);
  std::unique_ptr<range::range> solve_quadratic(Float a, range::range &b, range::range &c);
  std::vector<int> propagate_linear(std::vector<std::pair<int, Float>> &changes, bool impact_sol, bool impact_que,
                                    const uint32_t access = 7);
  std::vector<int> propagate_linear_quad(std::vector<std::pair<int, Float>> &changes, bool impact_sol, bool impact_que,
                                         const uint32_t access = 6);
  std::vector<int> propagate_quadratic(std::vector<std::pair<int, Float>> &changes, bool impact_sol, bool impact_que,
                                       const uint32_t access = 6);

  void apply_assignment();
  void remove_cons_fixvar(int cons, int vars);
  void add_cons_fixvar(int cons, int vars);

  std::unordered_map<int, Float> get_assignment() {
    std::unordered_map<int, Float> assignment;
    for (int i = 0; i < _assign_vars.size(); ++i) {
      int idx = _assign_vars[i];
      assignment.insert({idx, _prop_assignment[idx]});
    }
    return assignment;
  }

 private:
  Float calc_reason(int c, bool is_equal);
  void update_reason(int idx, bool is_dec);
  void ls_fixed_impact(int idx, range::range::status prev_status);
  void ls_impact(std::vector<int> &que, int idx, bool impact_sol, bool impact_que);
  void constant_push(std::vector<int> &que, int var_idx, bool impact_sol, range::range::status prev_status);
  bool update_qcons(std::vector<int> &que, bool impact_sol, bool impact_que);

  void buffer_store(int idx);
  void rollback();
  void clear_buffers();
  void clear_assignment();

  range_type _type;
  qp_solver &_solver;
  std::set<int> _fix_qcons;
  std::vector<std::set<int>> _cons_fixvar;
  std::vector<std::vector<int>> _cons, _qcons;
  std::vector<Float> _reason_score, _prop_assignment;
  std::vector<std::unique_ptr<range::range>> _ranges, _buffers;
  std::vector<int> _buffer_vars, _global_vars, _cons_freevars, _qcons_freevars, _assign_vars;
  std::vector<bool> _changed_vars, _global_changed_vars, _is_fixed, _buffer_fixed, _is_assigned;
};
}  // namespace solver