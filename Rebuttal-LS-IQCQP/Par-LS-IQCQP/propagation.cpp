#include "propagation.h"

#include <random>

namespace solver {
namespace range {

inline Float mul(Float x, Float y) {
  if (std::abs(x) <= range::eps || std::abs(y) <= range::eps) return 0;
  return x * y;
}

/*----------------------------------------------------------------------------*/

fuzzy_range::fuzzy_range(const var &v) { set(v); }

void fuzzy_range::set(const var &v) {
  _is_int = v.is_int || v.is_bin;
  _lower = v.has_lower ? v.lower : (v.is_bin ? 0 : -1e20);
  _upper = v.has_upper ? v.upper : (v.is_bin ? 1 : 1e20);
}

std::unique_ptr<range> fuzzy_range::operator+(const range &r) {
  Float lower = _lower + r.lower(), upper = _upper + r.upper();
  // assert(lower == lower && upper == upper);

  bool is_int = _is_int && r.is_int();
  if (is_int) {
    lower = eps_ceil(lower);
    upper = eps_floor(upper);
  }
  return std::make_unique<fuzzy_range>(lower, upper, is_int);
}

std::unique_ptr<range> fuzzy_range::operator-(const range &r) {
  Float lower = _lower - r.upper(), upper = _upper - r.lower();
  // assert(lower == lower && upper == upper);

  bool is_int = _is_int && r.is_int();
  if (is_int) {
    lower = eps_ceil(lower);
    upper = eps_floor(upper);
  }
  return std::make_unique<fuzzy_range>(lower, upper, is_int);
}

std::unique_ptr<range> fuzzy_range::operator*(const range &r) {
  Float lower = std::min(std::min(mul(_lower, r.lower()), mul(_lower, r.upper())),
                         std::min(mul(_upper, r.lower()), mul(_upper, r.upper()))),
        upper = std::max(std::max(mul(_lower, r.lower()), mul(_lower, r.upper())),
                         std::max(mul(_upper, r.lower()), mul(_upper, r.upper())));
  // assert(lower == lower && upper == upper);

  bool is_int = _is_int && r.is_int();
  if (is_int) {
    lower = eps_ceil(lower);
    upper = eps_floor(upper);
  }
  return std::make_unique<fuzzy_range>(lower, upper, is_int);
}

std::unique_ptr<range> fuzzy_range::operator*(Float n) {
  Float lower = mul(_lower, n), upper = mul(_upper, n);
  // assert(lower == lower && upper == upper);

  bool is_int = false;
  if (std::abs(n) <= eps)
    return std::make_unique<fuzzy_range>(0, 0, is_int);
  else if (n < -eps)
    return std::make_unique<fuzzy_range>(upper, lower, is_int);
  else
    return std::make_unique<fuzzy_range>(lower, upper, is_int);
}

std::unique_ptr<range> fuzzy_range::square() {
  Float lower = _lower <= eps && -eps <= _upper ? 0 : std::min(mul(_lower, _lower), mul(_upper, _upper)),
        upper = std::max(mul(_lower, _lower), mul(_upper, _upper));
  // assert(lower == lower && upper == upper);

  bool is_int = _is_int;
  if (is_int) {
    lower = eps_ceil(lower);
    upper = eps_floor(upper);
  }
  return std::make_unique<fuzzy_range>(lower, upper, is_int);
}

range::status fuzzy_range::get_status() {
  if (!range_valid(_lower, _upper)) return range::status::INVALID;

  if (std::abs(_lower - _upper) <= eps) return range::status::CONSTANT;

  if (_lower - _upper > eps) return range::status::INVALID;
  return range::status::VALID;
}

range::status fuzzy_range::intersect(range &r) {
  _lower = std::max(_lower, r.lower());
  _upper = std::min(_upper, r.upper());
  if (_is_int) {
    _lower = eps_ceil(_lower);
    _upper = eps_floor(_upper);
  }

  return get_status();
}

/*----------------------------------------------------------------------------*/

quad_fuzzy_range::quad_fuzzy_range(const var &v) { set(v); }

void quad_fuzzy_range::add_range(std::list<std::pair<Float, Float>> &ranges, const std::pair<Float, Float> &inr,
                                 std::list<std::pair<Float, Float>>::iterator &it, bool is_int) {
  std::pair<Float, Float> r = inr;
  if (is_int) {
    r.first = eps_ceil(r.first);
    r.second = eps_floor(r.second);
  }
  if (r.first - r.second > eps) return;

  if (ranges.empty()) {
    ranges.push_back(r);
    it = ranges.begin();
    return;
  }

  while (it != ranges.end() && it->second - r.first < -eps) ++it;
  if (it == ranges.end()) {
    ranges.push_back(r);
    --it;
    return;
  }

  if (it->first - r.second > eps) {
    it = ranges.insert(it, r);
    return;
  }

  it->first = std::min(it->first, r.first);
  it->second = std::max(it->second, r.second);
  while (it != ranges.end() && it->first - r.second <= eps) {
    it->first = std::min(it->first, r.first);
    it->second = std::max(it->second, r.second);
    auto it_next = it;
    ++it_next;
    if (it_next != ranges.end() && it_next->first - it->second <= eps) {
      it->second = std::max(it->second, it_next->second);
      it = ranges.erase(it_next);
    } else {
      break;
    }
  }
}

void quad_fuzzy_range::add_range(const range &r) {
  const auto &r_ranges = r.get_range();
  auto it = _f_ranges.begin();
  for (const auto &p : r_ranges) {
    add_range(_f_ranges, p, it, _is_int);
  }
}

std::unique_ptr<range> quad_fuzzy_range::operator+(const range &r) {
  std::list<std::pair<Float, Float>> new_ranges;
  const auto &r_ranges = r.get_range();
  bool is_int = _is_int && r.is_int();

  for (const auto &p1 : _f_ranges) {
    auto it = new_ranges.begin();
    for (const auto &p2 : r_ranges) {
      add_range(new_ranges, {p1.first + p2.first, p1.second + p2.second}, it, is_int);
    }
  }
  return std::make_unique<quad_fuzzy_range>(new_ranges, is_int);
}

std::unique_ptr<range> quad_fuzzy_range::operator-(const range &r) {
  std::list<std::pair<Float, Float>> new_ranges;
  const auto &r_ranges = r.get_range();
  bool is_int = _is_int && r.is_int();

  for (const auto &p1 : _f_ranges) {
    auto it = new_ranges.begin();
    for (auto p2 = r_ranges.rbegin(); p2 != r_ranges.rend(); ++p2) {
      add_range(new_ranges, {p1.first - p2->second, p1.second - p2->first}, it, is_int);
    }
  }

  return std::make_unique<quad_fuzzy_range>(new_ranges, is_int);
}

std::unique_ptr<range> quad_fuzzy_range::operator*(const range &r) {
  std::list<std::pair<Float, Float>> new_ranges;
  const auto &r_ranges = r.get_range();
  bool is_int = _is_int && r.is_int();

  for (const auto &p1 : _f_ranges) {
    for (const auto &p2 : r_ranges) {
      auto it = new_ranges.begin();
      add_range(new_ranges,
                {std::min(std::min(mul(p1.first, p2.first), mul(p1.first, p2.second)),
                          std::min(mul(p1.second, p2.first), mul(p1.second, p2.second))),
                 std::max(std::max(mul(p1.first, p2.first), mul(p1.first, p2.second)),
                          std::max(mul(p1.second, p2.first), mul(p1.second, p2.second)))},
                it, is_int);
    }
  }

  return std::make_unique<quad_fuzzy_range>(new_ranges, is_int);
}

std::unique_ptr<range> quad_fuzzy_range::operator*(Float n) {
  std::list<std::pair<Float, Float>> new_ranges;
  if (std::abs(n) <= eps) {
    new_ranges.emplace_back(0, 0);
    return std::make_unique<quad_fuzzy_range>(new_ranges, false);
  } else if (n < -eps) {
    for (auto p = _f_ranges.rbegin(); p != _f_ranges.rend(); ++p) {
      new_ranges.emplace_back(mul(p->second, n), mul(p->first, n));
    }
    return std::make_unique<quad_fuzzy_range>(new_ranges, false);
  } else {
    for (const auto &p : _f_ranges) {
      new_ranges.emplace_back(mul(p.first, n), mul(p.second, n));
    }
    return std::make_unique<quad_fuzzy_range>(new_ranges, false);
  }
}

std::unique_ptr<range> quad_fuzzy_range::square() {
  std::list<std::pair<Float, Float>> new_ranges;
  auto it = new_ranges.begin();
  for (const auto &p : _f_ranges)
    if (p.first > eps)
      add_range(new_ranges,
                {p.first <= eps && -eps <= p.second ? 0 : std::min(mul(p.first, p.first), mul(p.second, p.second)),
                 std::max(mul(p.first, p.first), mul(p.second, p.second))},
                it, _is_int);

  it = new_ranges.begin();
  for (auto p = _f_ranges.rbegin(); p != _f_ranges.rend(); ++p)
    if (p->first < -eps)
      add_range(
          new_ranges,
          {p->first <= eps && -eps <= p->second ? 0 : std::min(mul(p->first, p->first), mul(p->second, p->second)),
           std::max(mul(p->first, p->first), mul(p->second, p->second))},
          it, _is_int);

  return std::make_unique<quad_fuzzy_range>(new_ranges, _is_int);
}

void quad_fuzzy_range::set(const var &v) {
  Float lower = v.has_lower ? v.lower : (v.is_bin ? 0 : -1e20),
        upper = v.has_upper ? v.upper : (v.is_bin ? 1 : 1e20);

  _f_ranges.clear();
  _f_ranges.emplace_back(lower, upper);
  _is_int = v.is_int || v.is_bin;
}

bool quad_fuzzy_range::in_range(Float v) {
  if (_is_int && !(std::abs(v - std::round(v)) <= eps)) return false;
  for (const auto &p : _f_ranges) {
    if (p.first - v <= eps && v - p.second <= eps)
      return true;
    else if (p.first - v > eps)
      break;
  }
  return false;
}

range::status quad_fuzzy_range::get_status() {
  if (_f_ranges.empty()) return range::status::INVALID;
  for (auto &p : _f_ranges)
    if (!range_valid(p.first, p.second)) return range::status::INVALID;

  if (_f_ranges.size() == 1 && std::abs(_f_ranges.begin()->first - _f_ranges.begin()->second) <= eps)
    return range::status::CONSTANT;

  return range::status::VALID;
}

range::status quad_fuzzy_range::intersect(range &r) {
  std::list<std::pair<Float, Float>> new_ranges;
  const auto &r_ranges = r.get_range();
  auto it1 = _f_ranges.begin();
  auto it2 = r_ranges.begin();

  while (it1 != _f_ranges.end() && it2 != r_ranges.end()) {
    if (it1->second - it2->first < -eps) {
      ++it1;
    } else if (it2->second - it1->first < -eps) {
      ++it2;
    } else {
      Float lower = std::max(it1->first, it2->first), upper = std::min(it1->second, it2->second);
      if (_is_int) lower = eps_ceil(lower), upper = eps_floor(upper);
      if (lower - upper <= eps) new_ranges.emplace_back(lower, upper);

      if (it1->second - it2->second < -eps) {
        ++it1;
      } else {
        ++it2;
      }
    }
  }

  _f_ranges = std::move(new_ranges);
  return get_status();
}

/*----------------------------------------------------------------------------*/

exact_range::exact_range(const var &v) { set(v); }

void exact_range::add_range(std::list<std::pair<Float, Float>> &ranges, const std::pair<Float, Float> &r,
                            std::list<std::pair<Float, Float>>::iterator &it) {
  if (ranges.empty()) {
    ranges.push_back(r);
    it = ranges.begin();
    return;
  }

  while (it != ranges.end() && it->second - r.first < -eps) ++it;
  if (it == ranges.end()) {
    ranges.push_back(r);
    --it;
    return;
  }

  if (it->first - r.second > eps) {
    it = ranges.insert(it, r);
    return;
  }

  it->first = std::min(it->first, r.first);
  it->second = std::max(it->second, r.second);
  while (it != ranges.end() && it->first - r.second <= eps) {
    it->first = std::min(it->first, r.first);
    it->second = std::max(it->second, r.second);
    auto it_next = it;
    ++it_next;
    if (it_next != ranges.end() && it_next->first - it->second <= eps) {
      it->second = std::max(it->second, it_next->second);
      it = ranges.erase(it_next);
    } else {
      break;
    }
  }
}

void exact_range::add_range(const range &r) {
  const auto &r_ranges = r.get_range();
  auto it = _f_ranges.begin();
  for (const auto &p : r_ranges) {
    add_range(_f_ranges, p, it);
  }
}

std::unique_ptr<range> exact_range::operator+(const range &r) {
  std::list<std::pair<Float, Float>> new_ranges;
  const auto &r_ranges = r.get_range();
  for (const auto &p1 : _f_ranges) {
    auto it = new_ranges.begin();
    for (const auto &p2 : r_ranges) {
      add_range(new_ranges, {p1.first + p2.first, p1.second + p2.second}, it);
    }
  }
  return std::make_unique<exact_range>(new_ranges);
}

std::unique_ptr<range> exact_range::operator-(const range &r) {
  std::list<std::pair<Float, Float>> new_ranges;
  const auto &r_ranges = r.get_range();
  for (const auto &p1 : _f_ranges) {
    auto it = new_ranges.begin();
    for (auto p2 = r_ranges.rbegin(); p2 != r_ranges.rend(); ++p2) {
      add_range(new_ranges, {p1.first - p2->second, p1.second - p2->first}, it);
    }
  }

  return std::make_unique<exact_range>(new_ranges);
}

std::unique_ptr<range> exact_range::operator*(const range &r) {
  std::list<std::pair<Float, Float>> new_ranges;
  const auto &r_ranges = r.get_range();
  for (const auto &p1 : _f_ranges) {
    for (const auto &p2 : r_ranges) {
      auto it = new_ranges.begin();
      add_range(new_ranges,
                {std::min(std::min(mul(p1.first, p2.first), mul(p1.first, p2.second)),
                          std::min(mul(p1.second, p2.first), mul(p1.second, p2.second))),
                 std::max(std::max(mul(p1.first, p2.first), mul(p1.first, p2.second)),
                          std::max(mul(p1.second, p2.first), mul(p1.second, p2.second)))},
                it);
    }
  }

  return std::make_unique<exact_range>(new_ranges);
}

std::unique_ptr<range> exact_range::operator*(Float n) {
  std::list<std::pair<Float, Float>> new_ranges;
  if (std::abs(n) <= eps) {
    new_ranges.emplace_back(0, 0);
    return std::make_unique<exact_range>(new_ranges);
  } else if (n < -eps) {
    for (auto p = _f_ranges.rbegin(); p != _f_ranges.rend(); ++p) {
      new_ranges.emplace_back(mul(p->second, n), mul(p->first, n));
    }
    return std::make_unique<exact_range>(new_ranges);
  } else {
    for (const auto &p : _f_ranges) {
      new_ranges.emplace_back(mul(p.first, n), mul(p.second, n));
    }
    return std::make_unique<exact_range>(new_ranges);
  }
}

std::unique_ptr<range> exact_range::square() {
  std::list<std::pair<Float, Float>> new_ranges;
  auto it = new_ranges.begin();
  for (const auto &p : _f_ranges)
    if (p.first > eps)
      add_range(new_ranges,
                {p.first <= eps && -eps <= p.second ? 0 : std::min(mul(p.first, p.first), mul(p.second, p.second)),
                 std::max(mul(p.first, p.first), mul(p.second, p.second))},
                it);

  it = new_ranges.begin();
  for (auto p = _f_ranges.rbegin(); p != _f_ranges.rend(); ++p)
    if (p->first < -eps)
      add_range(
          new_ranges,
          {p->first <= eps && -eps <= p->second ? 0 : std::min(mul(p->first, p->first), mul(p->second, p->second)),
           std::max(mul(p->first, p->first), mul(p->second, p->second))},
          it);

  return std::make_unique<exact_range>(new_ranges);
}

void exact_range::set(const var &v) {
  Float lower = v.has_lower ? v.lower : (v.is_bin ? 0 : -1e20),
        upper = v.has_upper ? v.upper : (v.is_bin ? 1 : 1e20);

  _f_ranges.clear();
  if (v.is_int || v.is_bin) {
    if ((!v.has_lower || !v.has_upper) && !v.is_bin) {
      // [TODO] handle the case where the variable is binary and has no lower/upper bound
      _f_ranges.emplace_back(lower, upper);
    } else {
      int int_lower = eps_ceil(lower), int_upper = eps_floor(upper);
      for (int i = int_lower; i <= int_upper; ++i) {
        _f_ranges.emplace_back(static_cast<Float>(i), static_cast<Float>(i));
      }
    }
  } else {
    _f_ranges.emplace_back(lower, upper);
  }
}

bool exact_range::in_range(Float v) {
  for (const auto &p : _f_ranges) {
    if (p.first - v <= eps && v - p.second <= eps)
      return true;
    else if (p.first - v > eps)
      break;
  }
  return false;
}

range::status exact_range::get_status() {
  if (_f_ranges.empty()) return range::status::INVALID;
  for (auto &p : _f_ranges)
    if (!range_valid(p.first, p.second)) return range::status::INVALID;

  if (_f_ranges.size() == 1 && std::abs(_f_ranges.begin()->first - _f_ranges.begin()->second) <= eps)
    return range::status::CONSTANT;

  return range::status::VALID;
}

range::status exact_range::intersect(range &r) {
  std::list<std::pair<Float, Float>> new_ranges;
  const auto &r_ranges = r.get_range();
  auto it1 = _f_ranges.begin();
  auto it2 = r_ranges.begin();

  while (it1 != _f_ranges.end() && it2 != r_ranges.end()) {
    if (it1->second - it2->first < -eps) {
      ++it1;
    } else if (it2->second - it1->first < -eps) {
      ++it2;
    } else {
      new_ranges.emplace_back(std::max(it1->first, it2->first), std::min(it1->second, it2->second));
      if (it1->second - it2->second < -eps) {
        ++it1;
      } else {
        ++it2;
      }
    }
  }

  _f_ranges = std::move(new_ranges);
  return get_status();
}
}  // namespace range

/*----------------------------------------------------------------------------*/

qp_solver::propagation::propagation(qp_solver &solver, range_type type) : _solver(solver), _type(type) {
  _ranges.reserve(_solver._vars.size());
  for (auto &v : _solver._vars) {
    switch (type) {
      case range_type::FUZZY:
        _ranges.emplace_back(std::make_unique<range::fuzzy_range>(v));
        break;
      case range_type::QUAD_FUZZY:
        _ranges.emplace_back(std::make_unique<range::quad_fuzzy_range>(v));
        break;
      case range_type::EXACT:
        _ranges.emplace_back(std::make_unique<range::exact_range>(v));
        break;
      default:
        break;
    }
  }

  _cons.resize(_solver._vars.size());
  _qcons.resize(_solver._vars.size());
  _reason_score.resize(_solver._vars.size(), 0);
  _cons_freevars.resize(_solver._constraints.size(), 0);
  _qcons_freevars.resize(_solver._constraints.size(), 0);
  for (size_t i = 0; i < _solver._constraints.size(); ++i) {
    bool is_linear = true;
    auto &mon = *_solver._constraints[i].monomials;
    for (size_t j = 0; j < mon.size(); ++j) {
      if (!mon[j].is_linear) {
        is_linear = false;
        break;
      }
    }

    if (is_linear) {
      for (size_t j = 0; j < mon.size(); ++j) {
        int var_idx = mon[j].m_vars[0];
        _cons[var_idx].push_back(i);
        if (_ranges[var_idx]->get_status() != range::range::status::CONSTANT) _cons_freevars[i]++;
      }
    } else {
      std::set<int> vars;
      for (size_t j = 0; j < mon.size(); ++j)
        for (const auto &v : mon[j].m_vars) vars.insert(v);

      for (const int &v : vars) {
        _qcons[v].push_back(i);
        if (_ranges[v]->get_status() != range::range::status::CONSTANT) {
          if (_qcons_freevars[i] == 1) {
            if (_fix_qcons.find(i) != _fix_qcons.end()) _fix_qcons.erase(i);
#ifdef QUAD_LOG
            fprintf(stderr, "***[propagation]*** remove %s from _fix_qcons\n", _solver._constraints[i].name.c_str());
#endif
          }

          _qcons_freevars[i]++;

          if (_qcons_freevars[i] == 1) {
            _fix_qcons.insert(i);
#ifdef QUAD_LOG
            fprintf(stderr, "***[propagation]*** add %s to _fix_qcons\n", _solver._constraints[i].name.c_str());
#endif
          }
        }
      }
    }
  }
  for (size_t i = 0; i < _solver._vars.size(); ++i) {
    for (const int &c : _cons[i]) _reason_score[i] += calc_reason(_cons_freevars[c], _solver._constraints[c].is_equal);
  }

#ifdef INIT_LOG
  fprintf(stderr, "***[propagation]*** ls_score initialized\n");
  fprintf(stderr, "***[propagation]*** _cons_freevars: ");
  for (size_t i = 0; i < _cons_freevars.size(); ++i) {
    fprintf(stderr, "%d ", _cons_freevars[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "***[propagation]*** _qcons_freevars: ");
  for (size_t i = 0; i < _qcons_freevars.size(); ++i) {
    fprintf(stderr, "%d ", _qcons_freevars[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "***[propagation]*** _reason_score: ");
  for (size_t i = 0; i < _reason_score.size(); ++i) {
    fprintf(stderr, "%lf ", _reason_score[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "***[propagation]*** ranges:\n");
  for (size_t i = 0; i < _ranges.size(); ++i) {
    fprintf(stderr, "***[propagation]*** %s: ", _solver._vars[i].name.c_str());
    for (auto &p : _ranges[i]->get_range()) {
      fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
    }
    fprintf(stderr, "\n");
  }
#endif

  _buffer_vars.reserve(_solver._vars.size());

  _buffers.resize(_solver._vars.size());
  _changed_vars.resize(_solver._vars.size(), false);
  _buffer_fixed.resize(_solver._vars.size(), false);
  _is_fixed.resize(_solver._vars.size(), false);

  _global_vars.reserve(_solver._vars.size());
  _global_changed_vars.resize(_solver._vars.size(), false);

  _assign_vars.reserve(_solver._vars.size());
  _prop_assignment.resize(_solver._vars.size(), 0);
  _is_assigned.resize(_solver._vars.size(), false);

  _cons_fixvar.resize(_solver._constraints.size());
}

void qp_solver::propagation::rollback() {
#ifdef PROP_LOG
  fprintf(stderr, "***[rollback]*** rollback\n");
#endif

  assert(_changed_vars.size() == _buffers.size());
  for (size_t i = 0; i < _buffer_vars.size(); ++i) {
    int idx = _buffer_vars[i];
    assert(_changed_vars[idx] == true && _buffers[idx] != nullptr);

#ifdef PROP_LOG
    fprintf(stderr, "***[rollback]*** var %s rollback\n", _solver._vars[idx].name.c_str());
#endif

    bool need_update = _ranges[idx]->get_status() == range::range::status::CONSTANT;
    _changed_vars[idx] = false;
    _ranges[idx] = std::move(_buffers[idx]);
    _is_fixed[idx] = _buffer_fixed[idx];

    need_update &= _ranges[idx]->get_status() != range::range::status::CONSTANT;
    if (!need_update) continue;

#ifdef SCORE_LOG
    fprintf(stderr, "***[rollback]*** %s need to update reason\n", _solver._vars[idx].name.c_str());
#endif
    update_reason(idx, false);
  }
  _buffer_vars.clear();

  clear_assignment();
}

void qp_solver::propagation::clear_buffers() {
  for (size_t i = 0; i < _buffer_vars.size(); ++i) {
    _changed_vars[_buffer_vars[i]] = false;
    _buffer_fixed[_buffer_vars[i]] = false;
    _buffers[_buffer_vars[i]].reset();
  }
  _buffer_vars.clear();
}

void qp_solver::propagation::clear_assignment() {
#ifdef INIT_LOG
  fprintf(stderr, "***[clear_assignment]*** clear_assignment\n");
#endif

  for (size_t i = 0; i < _assign_vars.size(); ++i) {
    int idx = _assign_vars[i];
    _prop_assignment[idx] = 0;
    _is_assigned[idx] = false;
  }
  _assign_vars.clear();
}

void qp_solver::propagation::apply_assignment() {
#ifdef LS_LOG
  fprintf(stderr, "***[apply_assignment]*** apply_assignment\n");
#endif

  for (size_t i = 0; i < _assign_vars.size(); ++i) {
    int idx = _assign_vars[i];
    Float old_val = _solver._cur_assignment[idx];
    assert(_ranges[idx]->in_range(_prop_assignment[idx]));
    _solver.execute_critical_move_mix(idx, -old_val + _prop_assignment[idx]);
    // assert(std::abs(_solver._cur_assignment[idx] - _prop_assignment[idx]) <= range::range::eps);
#ifdef LS_LOG
    fprintf(stderr, "***[apply_assignment]*** var %s: %lf -> %lf, expected %lf\n", _solver._vars[idx].name.c_str(),
            old_val, _solver._cur_assignment[idx], _prop_assignment[idx]);
#endif
    _prop_assignment[idx] = 0;
    _is_assigned[idx] = false;
  }
  _assign_vars.clear();

#ifdef LS_LOG
  fprintf(stderr, "***[apply_assignment]*** now unsat constraints: %d\n", (int)_solver._unsat_constraints.size());
#endif
}

void qp_solver::propagation::init_ranges() {
  assert(_ranges.size() == _solver._vars.size());
  for (size_t i = 0; i < _global_vars.size(); ++i) {
    int idx = _global_vars[i];
    assert(_global_changed_vars[idx] == true);

#ifdef INIT_LOG
    fprintf(stderr, "***[init_ranges]*** var %s ranges(before init): ", _solver._vars[idx].name.c_str());
    for (auto &p : _ranges[idx]->get_range()) {
      fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "***[init_ranges]*** var %s status(before init): %d\n", _solver._vars[idx].name.c_str(),
            (int)_ranges[idx]->get_status());
#endif

    bool need_update = _ranges[idx]->get_status() == range::range::status::CONSTANT;
    _global_changed_vars[idx] = false;
    _ranges[idx]->set(_solver._vars[idx]);
    _changed_vars[idx] = false;
    _buffers[idx].reset();
    _is_fixed[idx] = false;

#ifdef INIT_LOG
    fprintf(stderr, "***[init_ranges]*** var %s ranges(after init): ", _solver._vars[idx].name.c_str());
    for (auto &p : _ranges[idx]->get_range()) {
      fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "***[init_ranges]*** var %s status(after init): %d\n", _solver._vars[idx].name.c_str(),
            (int)_ranges[idx]->get_status());
#endif

    need_update &= _ranges[idx]->get_status() != range::range::status::CONSTANT;
    if (!need_update) continue;

#ifdef INIT_LOG
    fprintf(stderr, "***[init_ranges]*** %s need to update\n", _solver._vars[idx].name.c_str());
#endif
    update_reason(idx, false);
  }
  _global_vars.clear();
  _buffer_vars.clear();

  clear_assignment();

#ifdef INIT_LOG
  fprintf(stderr, "***[init_ranges]*** ls_score initialized\n");
  fprintf(stderr, "***[init_ranges]*** _cons_freevars: ");
  for (size_t i = 0; i < _cons_freevars.size(); ++i) {
    fprintf(stderr, "%d ", _cons_freevars[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "***[init_ranges]*** _qcons_freevars: ");
  for (size_t i = 0; i < _qcons_freevars.size(); ++i) {
    fprintf(stderr, "%d ", _qcons_freevars[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "***[init_ranges]*** _reason_score: ");
  for (size_t i = 0; i < _reason_score.size(); ++i) {
    fprintf(stderr, "%lf ", _reason_score[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "***[init_ranges]*** ranges:\n");
  for (size_t i = 0; i < _ranges.size(); ++i) {
    fprintf(stderr, "***[init_ranges]*** %s: ", _solver._vars[i].name.c_str());
    for (auto &p : _ranges[i]->get_range()) {
      fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
    }
    fprintf(stderr, "\n");
  }
#endif
}

void qp_solver::propagation::remove_cons_fixvar(int cons, int vars) {
  assert(cons >= 0 && cons < (int)_cons_fixvar.size());
  if (_cons_fixvar[cons].find(vars) != _cons_fixvar[cons].end()) {
    _cons_fixvar[cons].erase(vars);
  }
}

void qp_solver::propagation::add_cons_fixvar(int cons, int vars) {
  assert(cons >= 0 && cons < (int)_cons_fixvar.size());
  _cons_fixvar[cons].insert(vars);
}

/*----------------------------------------------------------------------------*/

/**
 * @brief Computes the range of values for x that satisfy the inequality ax^2 +
 * bx + c >= 0.
 *
 * @param a The coefficient of x^2 in the quadratic inequality.
 * @param b The coefficient of x in the quadratic inequality.
 * @param c The constant term in the quadratic inequality.
 *
 * @return A std::unique_ptr to a range::range object representing the intervals
 * of x values that satisfy the inequality. If the inequality is always
 * satisfied or never satisfied, the range may represent the entire real line or
 * be empty, respectively.
 */
std::unique_ptr<range::range> qp_solver::propagation::solve_quadratic(Float a, Float b, Float c) {
  range::exact_range ranges(std::list<std::pair<Float, Float>>{});

  // handle cases with infinite parameters
  if (c == -std::numeric_limits<Float>::infinity()) {
    if (b == -std::numeric_limits<Float>::infinity()) {
      ranges.add_range(range::exact_range(-std::numeric_limits<Float>::infinity(), 0));
    } else if (b == std::numeric_limits<Float>::infinity()) {
      ranges.add_range(range::exact_range(0, std::numeric_limits<Float>::infinity()));
    }
    return std::make_unique<range::exact_range>(ranges.get_range());
  } else if (c == std::numeric_limits<Float>::infinity()) {
    ranges.add_range(
        range::exact_range(-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()));
    return std::make_unique<range::exact_range>(ranges.get_range());
  }
  if (b == -std::numeric_limits<Float>::infinity()) {
    ranges.add_range(range::exact_range(-std::numeric_limits<Float>::infinity(), 0));
    return std::make_unique<range::exact_range>(ranges.get_range());
  } else if (b == std::numeric_limits<Float>::infinity()) {
    ranges.add_range(range::exact_range(0, std::numeric_limits<Float>::infinity()));
    return std::make_unique<range::exact_range>(ranges.get_range());
  }

  if (std::abs(a) <= range::range::eps) {
    // handle cases with linear inequality

    if (std::abs(b) <= range::range::eps) {
      if (c >= -range::range::eps) {
        ranges.add_range(
            range::exact_range(-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()));
      }
    } else {
      Float root = -c / b;
      if (b > range::range::eps) {
        ranges.add_range(range::exact_range(root, std::numeric_limits<Float>::infinity()));
      } else {
        ranges.add_range(range::exact_range(-std::numeric_limits<Float>::infinity(), root));
      }
    }
  } else {
    // handle cases with quadratic inequality

    Float root[2];
    int num_roots = gsl_poly_solve_quadratic(a, b, c, &root[0], &root[1]);
    if (num_roots == 0) {
      if (a > range::range::eps) {
        ranges.add_range(
            range::exact_range(-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()));
      }
    } else if (num_roots == 1 || std::abs(root[0] - root[1]) <= range::range::eps) {
      if (a > range::range::eps) {
        ranges.add_range(
            range::exact_range(-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()));
      } else {
        ranges.add_range(range::exact_range(root[0], root[0]));
      }
    } else {
      if (root[0] > root[1]) std::swap(root[0], root[1]);
      if (a > range::range::eps) {
        ranges.add_range(range::exact_range(-std::numeric_limits<Float>::infinity(), root[0]));
        ranges.add_range(range::exact_range(root[1], std::numeric_limits<Float>::infinity()));
      } else {
        ranges.add_range(range::exact_range(root[0], root[1]));
      }
    }
  }

  return std::make_unique<range::exact_range>(ranges.get_range());
}

/**
 * @brief Computes the range of values for x that satisfy the quadratic equation
 * ax^2 + bx + c = 0, where the coefficients b and c are specified as ranges of
 * values.
 *
 * @param a The coefficient of x^2 in the quadratic equation, given as a fixed
 * Float value.
 * @param b A reference to a range::range object representing the range of
 * possible values for the coefficient of x in the quadratic equation.
 * @param c A reference to a range::range object representing the range of
 * possible values for the constant term in the quadratic equation.
 *
 * @return A std::unique_ptr to a range::range object representing the range of
 * x values that are solutions to the equation for any combination of b and c
 * within their respective ranges. If no real solutions exist for any
 * combination, the range may be empty.
 */

std::unique_ptr<range::range> qp_solver::propagation::solve_quadratic(Float a, range::range &b, range::range &c) {
  const auto &b_ranges = b.get_range(), &c_ranges = c.get_range();

  range::exact_range ranges(std::list<std::pair<Float, Float>>{});
  for (const auto &p1 : b_ranges) {
    for (const auto &p2 : c_ranges) {
      // -a * x^2 - b * x = c

      // case 1: c.lower() <= -a * x^2 - b.lower() * x <= c.upper()
      std::unique_ptr<range::range> domain_left = solve_quadratic(-a, -p1.first, -p2.first),
                                    domain_right = solve_quadratic(a, p1.first, p2.second);
      domain_left->intersect(*domain_right);
      ranges.add_range(*domain_left);

      // case 2: c.lower() <= -a * x^2 - b.upper() * x <= c.upper()
      domain_left = solve_quadratic(-a, -p1.second, -p2.first);
      domain_right = solve_quadratic(a, p1.second, p2.second);
      domain_left->intersect(*domain_right);
      ranges.add_range(*domain_left);

      // case 3: -a * x^2 - b.lower() * x <= c.lower() <= c.upper() <= -a * x^2 - b.upper() * x
      domain_left = solve_quadratic(a, p1.first, p2.first);
      domain_right = solve_quadratic(-a, -p1.second, -p2.second);
      domain_left->intersect(*domain_right);
      ranges.add_range(*domain_left);

      // case 4: -a * x^2 - b.upper() * x <= c.lower() <= c.upper() <= -a * x^2 - b.lower() * x
      domain_left = solve_quadratic(a, p1.second, p2.first);
      domain_right = solve_quadratic(-a, -p1.first, -p2.second);
      domain_left->intersect(*domain_right);
      ranges.add_range(*domain_left);
    }
  }

  return std::make_unique<range::exact_range>(ranges.get_range());
}

void qp_solver::propagation::buffer_store(int idx) {
#ifdef PROP_LOG
  fprintf(stderr, "***[buffer_store]*** now at %s\n", _solver._vars[idx].name.c_str());
  fprintf(stderr, "***[buffer_store]*** range: ");
  for (auto &p : _ranges[idx]->get_range()) {
    fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
  }
  fprintf(stderr, "\n");
#endif
  if (!_changed_vars[idx]) {
    _buffers[idx] = _ranges[idx]->copy();
    _buffer_fixed[idx] = _is_fixed[idx];
    _changed_vars[idx] = true;
    _buffer_vars.emplace_back(idx);
  }
  if (!_global_changed_vars[idx]) {
    _global_vars.emplace_back(idx);
    _global_changed_vars[idx] = true;
  }
}

std::unique_ptr<range::range> qp_solver::propagation::gen(Float lower, Float upper) {
  switch (_type) {
    case range_type::FUZZY:
      return std::make_unique<range::fuzzy_range>(lower, upper, false);
    case range_type::QUAD_FUZZY:
      return std::make_unique<range::quad_fuzzy_range>(lower, upper, false);
    case range_type::EXACT:
      return std::make_unique<range::exact_range>(lower, upper);
    default:
      return std::make_unique<range::exact_range>(lower, upper);
  }
}

Float qp_solver::propagation::calc_reason(int c, bool is_equal) {
  assert(c >= 0);

  // [TODO] use boost to speed up

  switch (c) {
    case 0:
      return 0;
    case 1:
      return 100;
    case 2:
      return 10;
    case 3:
      return 1;
    case 4:
    case 5:
    case 6:
    case 7:
      return (Float)1.0 / (c * std::sqrt((Float)c));
    default:
      return (Float)1.0 / (c * c);
  }
}

// [TODO] add a parameter to toggle the 'update_reason' function
void qp_solver::propagation::update_reason(int idx, bool is_dec) {
#ifdef SCORE_LOG
  fprintf(stderr, "***[update_reason]*** now at %s, is_dec: %d\n", _solver._vars[idx].name.c_str(), is_dec);
#endif

#ifdef QUAD_LOG
  fprintf(stderr, "***[update_reason]*** now at %s, is_dec: %d\n", _solver._vars[idx].name.c_str(), is_dec);
#endif

  for (const int &c : _qcons[idx]) {
    if (_qcons_freevars[c] == 1) {
      if (_fix_qcons.find(c) != _fix_qcons.end()) _fix_qcons.erase(c);
#ifdef QUAD_LOG
      fprintf(stderr, "***[update_reason]*** remove %s from _fix_qcons\n", _solver._constraints[c].name.c_str());
#endif
    }

#ifdef QUAD_LOG
    fprintf(stderr, "***[update_reason]*** now at constraint %d\n", c);
    fprintf(stderr, "***[update_reason]*** freevars before: %d, after: %d\n", _qcons_freevars[c],
            _qcons_freevars[c] + (is_dec ? -1 : 1));
#endif
    if (is_dec)
      _qcons_freevars[c]--;
    else
      _qcons_freevars[c]++;

    if (_qcons_freevars[c] == 1) {
      _fix_qcons.insert(c);
#ifdef QUAD_LOG
      fprintf(stderr, "***[update_reason]*** add %s to _fix_qcons\n", _solver._constraints[c].name.c_str());
#endif
    }
  }

  for (const int &c : _cons[idx]) {
    Float old_score = calc_reason(_cons_freevars[c], _solver._constraints[c].is_equal);
#ifdef SCORE_LOG
    fprintf(stderr, "***[update_reason]*** now at constraint %d\n", c);
    fprintf(stderr, "***[update_reason]*** freevars before: %d, after: %d\n", _cons_freevars[c],
            _cons_freevars[c] + (is_dec ? -1 : 1));
    fprintf(stderr, "***[update_reason]*** score before: %lf, after: %lf\n", old_score,
            calc_reason(_cons_freevars[c] + (is_dec ? -1 : 1), _solver._constraints[c].is_equal));
#endif

    if (is_dec)
      _cons_freevars[c]--;
    else
      _cons_freevars[c]++;
    Float delt = -old_score + calc_reason(_cons_freevars[c], _solver._constraints[c].is_equal);

    const auto &mons = *_solver._constraints[c].monomials;
    for (size_t j = 0; j < mons.size(); ++j) {
      int var_idx = mons[j].m_vars[0];
      _reason_score[var_idx] += delt;

#ifdef SCORE_LOG
      fprintf(stderr, "***[update_reason]*** update %s, before: %lf, after: %lf\n", _solver._vars[var_idx].name.c_str(),
              _reason_score[var_idx] - delt, _reason_score[var_idx]);
#endif
    }
  }
}

void qp_solver::propagation::ls_impact(std::vector<int> &que, int idx, bool impact_sol, bool impact_que) {
  if (!impact_sol) return;
  Float value = _is_assigned[idx] ? _prop_assignment[idx] : _solver._cur_assignment[idx];

#ifdef IMPACT_LOG
  fprintf(stderr, "***[ls_impact]*** now at %s, impact_sol: %d, impact_que: %d\n", _solver._vars[idx].name.c_str(),
          impact_sol, impact_que);
  fprintf(stderr, "***[ls_impact]*** current assignment: %lf\n", value);
  fprintf(stderr, "***[ls_impact]*** range: ");
  for (auto &p : _ranges[idx]->get_range()) {
    fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
  }
  fprintf(stderr, "\n");
#endif

  if (var_in_range(idx, value)) {
#ifdef IMPACT_LOG
    fprintf(stderr, "***[ls_impact]*** var %s is in range\n", _solver._vars[idx].name.c_str());
#endif

    return;
  }

#ifdef IMPACT_LOG
  fprintf(stderr, "***[ls_impact]*** var %s is not in range\n", _solver._vars[idx].name.c_str());
#endif

  const std::list<std::pair<Float, Float>> &range = _ranges[idx]->get_range();
  Float min_dist = std::numeric_limits<Float>::infinity(), min_value = value;
  for (const auto &p : range) {
    Float dist = std::abs(p.first - value);
    if (dist < min_dist) {
      min_dist = dist;
      min_value = p.first;
    }
    dist = std::abs(p.second - value);
    if (dist < min_dist) {
      min_dist = dist;
      min_value = p.second;
    }
  }
  _prop_assignment[idx] = min_value;
  if (!_is_assigned[idx]) {
    _is_assigned[idx] = true;
    _assign_vars.push_back(idx);
  }

#ifdef IMPACT_LOG
  fprintf(stderr, "***[apply_assignment]*** old value: %lf, new assignment: %lf, expected %lf\n", value,
          _prop_assignment[idx], min_value);
#endif

  if (impact_que) {
#ifdef IMPACT_LOG
    fprintf(stderr, "***[ls_impact]*** add %s to que\n", _solver._vars[idx].name.c_str());
#endif
    que.push_back(idx);
  }
}

void qp_solver::propagation::ls_fixed_impact(int idx, range::range::status prev_status) {
#ifdef IMPACT_LOG
  fprintf(stderr, "***[ls_fixed_impact]*** now at %s\n", _solver._vars[idx].name.c_str());
#endif

  if (prev_status == range::range::status::CONSTANT) return;

#ifdef IMPACT_LOG
  fprintf(stderr, "***[ls_fixed_impact]*** %s is first time to be fixed, start update reason\n",
          _solver._vars[idx].name.c_str());
#endif
  update_reason(idx, true);
}

void qp_solver::propagation::constant_push(std::vector<int> &que, int var_idx, bool impact_sol,
                                           range::range::status prev_status) {
  ls_fixed_impact(var_idx, prev_status);
  ls_impact(que, var_idx, impact_sol, false);
  if (!_is_fixed[var_idx]) {
    _is_fixed[var_idx] = true;
    que.push_back(var_idx);
#ifdef PROP_LOG
    fprintf(stderr, "***[constant_push]*** add %s to que\n", _solver._vars[var_idx].name.c_str());
#endif
  }
}

std::vector<int> qp_solver::propagation::propagate_linear(std::vector<std::pair<int, Float>> &changes, bool impact_sol,
                                                          bool impact_que, const uint32_t access) {
#ifdef PROP_LOG
  fprintf(stderr, "***[propagate_linear]*** start\n");
  fprintf(stderr, "***[propagate_linear]*** _is_fixed: ");
  for (size_t i = 0; i < _is_fixed.size(); ++i) {
    fprintf(stderr, "%d ", (int)_is_fixed[i]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "***[propagate_linear]*** ranges:\n");
  for (size_t i = 0; i < _ranges.size(); ++i) {
    fprintf(stderr, "***[propagate_linear]*** %s: ", _solver._vars[i].name.c_str());
    for (auto &p : _ranges[i]->get_range()) {
      fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "***[propagate_linear]*** cons:\n");
  for (size_t i = 0; i < _cons.size(); ++i) {
    fprintf(stderr, "***[propagate_linear]*** %s: ", _solver._vars[i].name.c_str());
    for (const int &c : _cons[i]) {
      fprintf(stderr, "%d ", c);
    }
    fprintf(stderr, "\n");
  }
#endif

  assert(access >> (uint32_t)_type & 1);
  if (impact_sol) clear_assignment();
  std::vector<int> que;
  for (int i = 0; i < changes.size(); ++i) {
    int idx = changes[i].first;
    Float val = changes[i].second;
    assert(0 <= idx && idx < _solver._vars.size());

#ifdef PROP_LOG
    fprintf(stderr, "***[propagate_linear]*** initial var %s to %lf\n", _solver._vars[idx].name.c_str(), val);
#endif

    buffer_store(idx);
    range::range::status prev_status = _ranges[idx]->get_status();
    if (_ranges[idx]->copy()->intersect(*gen(val, val)) == range::range::status::INVALID) {
      rollback();
      return {};
    }
    _ranges[idx]->intersect(*gen(val, val));
    constant_push(que, idx, impact_sol, prev_status);
  }

  for (int qindex = 0; qindex < que.size(); ++qindex) {
    int idx = que[qindex];
    if (_solver.TimeElapsed() > _solver._cut_off || _solver.terminate) {
      rollback();
      return {};
    }

    var *p_var = &_solver._vars[idx];
    for (const int c : _cons[idx]) {
      // Only consider linear constraints
      polynomial_constraint *p_con = &(_solver._constraints[c]);
      size_t size = p_con->monomials->size();

      // initialize bound and sum
      bool all_fixed = true;
      std::unique_ptr<range::range> bound, sum;
      Float lower = -std::numeric_limits<Float>::infinity(), upper = std::numeric_limits<Float>::infinity();
      if (p_con->is_equal)
        lower = upper = p_con->bound;
      else {
        if (p_con->is_less)
          upper = p_con->bound;
        else
          lower = p_con->bound;
      }
      sum = gen(0, 0);
      bound = gen(lower, upper);

#ifdef PROP_LOG
      fprintf(stderr, "***[propagate_linear]*** now at %s / %s\n", _solver._constraints[c].name.c_str(),
              _solver._vars[idx].name.c_str());
      fprintf(stderr, "***[propagate_linear]*** bound: ");
      for (auto &p : bound->get_range()) {
        fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
      }
      fprintf(stderr, "\n");

      fprintf(stderr, "***[propagate_linear]*** monomials: ");
      for (size_t i = 0; i < size; ++i) {
        int var_idx = p_con->monomials->at(i).m_vars[0];
        fprintf(stderr, "( %lf , %s , ", p_con->monomials->at(i).coeff, _solver._vars[var_idx].name.c_str());
        for (auto &p : _ranges[var_idx]->get_range()) {
          fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
        }
        fprintf(stderr, ") ");
      }
      fprintf(stderr, "\n");
#endif

      // check if the constraint is valid
      for (size_t i = 0; i < size; ++i) {
        assert(p_con->monomials->at(i).is_linear);

        int var_idx = p_con->monomials->at(i).m_vars[0];
        assert(!_is_fixed[var_idx] || _ranges[var_idx]->get_status() == range::range::status::CONSTANT);
        all_fixed &= _is_fixed[var_idx];
        sum = *sum + *(*_ranges[var_idx] * p_con->monomials->at(i).coeff);
        if (sum->get_status() == range::range::status::INVALID) {
          rollback();
          return {};
        }
      }
      sum->intersect(*bound);
      if (sum->get_status() == range::range::status::INVALID) {
        rollback();
        return {};
      }
      if (all_fixed) continue;

      // calculate prefix range
      std::vector<std::unique_ptr<range::range>> prefix(size), suffix(size);
      sum = gen(0, 0);
      for (size_t i = 0; i < size; ++i) {
        assert(p_con->monomials->at(i).is_linear);

        int var_idx = p_con->monomials->at(i).m_vars[0];
        if (_cons_fixvar[c].count(var_idx))
          sum = *sum +
                *(*gen(_solver._cur_assignment[var_idx], _solver._cur_assignment[var_idx]) * p_con->monomials->at(i).coeff);
        else
          sum = *sum + *(*_ranges[var_idx] * p_con->monomials->at(i).coeff);
        prefix[i] = sum->copy();

#ifdef PROP_LOG
        fprintf(stderr, "***[propagate_linear]*** prefix %d: ", (int)i);
        for (auto &p : prefix[i]->get_range()) {
          fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
        }
        fprintf(stderr, "\n");
#endif
      }

      // calculate suffix range
      sum = gen(0, 0);
      for (int i = size - 1; i >= 0; --i) {
        assert(p_con->monomials->at(i).is_linear);

        int var_idx = p_con->monomials->at(i).m_vars[0];
        if (_cons_fixvar[c].count(var_idx))
          sum = *sum +
                *(*gen(_solver._cur_assignment[var_idx], _solver._cur_assignment[var_idx]) * p_con->monomials->at(i).coeff);
        else
          sum = *sum + *(*_ranges[var_idx] * p_con->monomials->at(i).coeff);
        suffix[i] = sum->copy();

#ifdef PROP_LOG
        fprintf(stderr, "***[propagate_linear]*** suffix %d: ", i);
        for (auto &p : suffix[i]->get_range()) {
          fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
        }
        fprintf(stderr, "\n");
#endif
      }

      // calculate every variable's range in the constraint
      for (int i = 0; i < size; ++i) {
        int var_idx = p_con->monomials->at(i).m_vars[0];
        if (_cons_fixvar[c].count(var_idx)) continue;

        std::unique_ptr<range::range> domain = bound->copy();
        if (i > 0) domain = *domain - *prefix[i - 1];
        if (i + 1 < size) domain = *domain - *suffix[i + 1];
        domain = *domain * (1 / p_con->monomials->at(i).coeff);

        buffer_store(var_idx);
        range::range::status prev_status = _ranges[var_idx]->get_status();
#ifdef PROP_LOG
        auto temp = _ranges[var_idx]->copy();
        temp->intersect(*domain);
        fprintf(stderr, "***[propagate_linear]*** (now at %s / %s) after intersect variable %s range: ",
                _solver._constraints[c].name.c_str(), _solver._vars[idx].name.c_str(),
                _solver._vars[var_idx].name.c_str());
        for (auto &p : temp->get_range()) {
          fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
        }
        fprintf(stderr, ", domain: ");
        for (auto &p : domain->get_range()) {
          fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "***[propagate_linear]*** bound: ");
        for (auto &p : bound->get_range()) {
          fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
        }
        if (i > 0) {
          fprintf(stderr, ", prefix %d: ", i - 1);
          for (auto &p : prefix[i - 1]->get_range()) {
            fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
          }
        }
        if (i + 1 < size) {
          fprintf(stderr, ", suffix %d: ", i + 1);
          for (auto &p : suffix[i + 1]->get_range()) {
            fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
          }
        }
        fprintf(stderr, "\n");
#endif

        if (_ranges[var_idx]->copy()->intersect(*domain) == range::range::status::INVALID) {
          rollback();
          return {};
        }

        Float lower_delt = _ranges[var_idx]->lower(), upper_delt = _ranges[var_idx]->upper();
        range::range::status status = _ranges[var_idx]->intersect(*domain);
        lower_delt -= _ranges[var_idx]->lower(), upper_delt -= _ranges[var_idx]->upper();
        if (status == range::range::status::CONSTANT) {
          constant_push(que, var_idx, impact_sol, prev_status);
        } else {
          ls_impact(que, var_idx, impact_sol, impact_que);
        }
      }
    }
  }

#ifdef PROP_LOG
  fprintf(stderr, "***[propagate_linear]*** propagate successfully\n");
  fprintf(stderr, "***[propagate_linear]*** ranges:\n");
  for (size_t i = 0; i < _ranges.size(); ++i) {
    fprintf(stderr, "***[propagate_linear]*** %s: ", _solver._vars[i].name.c_str());
    for (auto &p : _ranges[i]->get_range()) {
      fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
    }
    fprintf(stderr, "\n");
  }
  fprintf(stderr, "***[propagate_linear]*** que: ");
  for (size_t i = 0; i < que.size(); ++i) {
    fprintf(stderr, "%s ", _solver._vars[que[i]].name.c_str());
  }
  fprintf(stderr, "\n");
#endif

  clear_buffers();
  return que;
}

bool qp_solver::propagation::update_qcons(std::vector<int> &que, bool impact_sol, bool impact_que) {
#ifdef QUAD_LOG
  fprintf(stderr, "***[update_qcons]*** start\n");
  fprintf(stderr, "***[update_qcons]*** _fix_qcons: ");
  for (const int &c : _fix_qcons) {
    fprintf(stderr, "%s ", _solver._constraints[c].name.c_str());
  }
  fprintf(stderr, "\n");
#endif

  std::vector<int> all_qcons;
  for (const int qc : _fix_qcons) all_qcons.push_back(qc);

  for (const int qc : all_qcons) {
    if (!_fix_qcons.count(qc)) continue;

    polynomial_constraint *p_con = &(_solver._constraints[qc]);
#ifdef QUAD_LOG
    fprintf(stderr, "***[update_qcons]*** now at %s\n", p_con->name.c_str());
#endif

    // initialize bound and sum
    std::unique_ptr<range::range> bound, sum;
    Float lower = -std::numeric_limits<Float>::infinity(), upper = std::numeric_limits<Float>::infinity();
    if (p_con->is_equal)
      lower = upper = p_con->bound;
    else {
      if (p_con->is_less)
        upper = p_con->bound;
      else
        lower = p_con->bound;
    }
    sum = gen(0, 0);
    bound = gen(lower, upper);

    // check if the constraint is fixed
    size_t size = p_con->monomials->size();
    int var_idx = -1;

    for (size_t i = 0; i < size; ++i) {
      for (const auto &v : p_con->monomials->at(i).m_vars) {
        if (_is_fixed[v]) continue;

        assert(var_idx == -1 || var_idx == v);
        var_idx = v;
      }
    }

#ifdef QUAD_LOG
    fprintf(stderr, "***[update_qcons]*** now fix variable %s\n", _solver._vars[var_idx].name.c_str());
#endif

    // calculate every variable's range in the constraint
    auto coeff = p_con->var_coeff->at(var_idx);
    std::unique_ptr<range::range> b = gen(coeff.obj_constant_coeff, coeff.obj_constant_coeff), c = *gen(0, 0) - *bound;
    for (int i = 0; i < coeff.obj_linear_coeff.size(); ++i) {
      int new_var = coeff.obj_linear_coeff[i];
      Float var_coeff = coeff.obj_linear_constant_coeff[i];
      b = *b + *(*_ranges[new_var] * var_coeff);
    }
    for (auto &m : *p_con->monomials) {
      if (m.m_vars.size() == 2) {
        int m_var_idx_1 = m.m_vars[0], m_var_idx_2 = m.m_vars[1];
        Float coeff = m.coeff;
        if (m_var_idx_1 != var_idx && m_var_idx_2 != var_idx)
          c = *c + *(*(*_ranges[m_var_idx_1] * *_ranges[m_var_idx_2]) * coeff);
      } else if (m.is_linear) {
        int m_var_idx = m.m_vars[0];
        Float coeff = m.coeff;
        if (m_var_idx != var_idx) c = *c + *(*_ranges[m_var_idx] * coeff);
      } else {
        int m_var_idx = m.m_vars[0];
        Float coeff = m.coeff;
        if (m_var_idx != var_idx) c = *c + *(*_ranges[m_var_idx]->square() * coeff);
      }

#ifdef QUAD_LOG
      fprintf(stderr, "***[update_qcons]*** now calculate monomials\n");
      fprintf(stderr, "***[update_qcons]*** a: %lf\n", coeff.obj_quadratic_coeff);
      fprintf(stderr, "***[update_qcons]*** b: ");
      for (auto &p : b->get_range()) {
        fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
      }
      fprintf(stderr, "\n");
      fprintf(stderr, "***[update_qcons]*** c: ");
      for (auto &p : c->get_range()) {
        fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
      }
      fprintf(stderr, "\n");
#endif
    }

    std::unique_ptr<range::range> domain = solve_quadratic(coeff.obj_quadratic_coeff, *b, *c);
    buffer_store(var_idx);
    range::range::status prev_status = _ranges[var_idx]->get_status();
#ifdef QUAD_LOG
    auto temp = _ranges[var_idx]->copy();
    temp->intersect(*domain);
    fprintf(stderr, "***[update_qcons]*** after intersect variable %s range: ", _solver._vars[var_idx].name.c_str());
    for (auto &p : temp->get_range()) {
      fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
    }
    fprintf(stderr, "\n");
#endif

    if (_ranges[var_idx]->copy()->intersect(*domain) == range::range::status::INVALID) {
      return false;
    }

    range::range::status status = _ranges[var_idx]->intersect(*domain);
    if (status == range::range::status::CONSTANT) {
      constant_push(que, var_idx, impact_sol, prev_status);
    } else {
      ls_impact(que, var_idx, impact_sol, impact_que);
    }

    if (_fix_qcons.find(qc) != _fix_qcons.end()) _fix_qcons.erase(qc);
  }

  if (_fix_qcons.empty())
    return true;
  else
    return update_qcons(que, impact_sol, impact_que);
}

std::vector<int> qp_solver::propagation::propagate_linear_quad(std::vector<std::pair<int, Float>> &changes,
                                                               bool impact_sol, bool impact_que,
                                                               const uint32_t access) {
  assert(access >> (uint32_t)_type & 1);
  if (impact_sol) clear_assignment();
  std::vector<int> que;
  for (int i = 0; i < changes.size(); ++i) {
    int idx = changes[i].first;
    Float val = changes[i].second;
    assert(0 <= idx && idx < _solver._vars.size());

    buffer_store(idx);
    range::range::status prev_status = _ranges[idx]->get_status();
    if (_ranges[idx]->copy()->intersect(*gen(val, val)) == range::range::status::INVALID) {
      rollback();
      return {};
    }
    _ranges[idx]->intersect(*gen(val, val));
    constant_push(que, idx, impact_sol, prev_status);
  }

  for (int qindex = 0; qindex < que.size(); ++qindex) {
    int idx = que[qindex];

    var *p_var = &_solver._vars[idx];
    for (const int c : _cons[idx]) {
      // Only consider linear constraints
      polynomial_constraint *p_con = &(_solver._constraints[c]);
      size_t size = p_con->monomials->size();

      // initialize bound and sum
      bool all_fixed = true;
      std::unique_ptr<range::range> bound, sum;
      Float lower = -std::numeric_limits<Float>::infinity(), upper = std::numeric_limits<Float>::infinity();
      if (p_con->is_equal)
        lower = upper = p_con->bound;
      else {
        if (p_con->is_less)
          upper = p_con->bound;
        else
          lower = p_con->bound;
      }
      sum = gen(0, 0);
      bound = gen(lower, upper);

      // check if the constraint is valid
      for (size_t i = 0; i < size; ++i) {
        assert(p_con->monomials->at(i).is_linear);

        int var_idx = p_con->monomials->at(i).m_vars[0];
        assert(!_is_fixed[var_idx] || _ranges[var_idx]->get_status() == range::range::status::CONSTANT);
        all_fixed &= _is_fixed[var_idx];
        sum = *sum + *(*_ranges[var_idx] * p_con->monomials->at(i).coeff);
        if (sum->get_status() == range::range::status::INVALID) {
          rollback();
          return {};
        }
      }
      sum->intersect(*bound);
      if (sum->get_status() == range::range::status::INVALID) {
        rollback();
        return {};
      }
      if (all_fixed) continue;

      // calculate prefix range
      std::vector<std::unique_ptr<range::range>> prefix(size), suffix(size);
      sum = gen(0, 0);
      for (size_t i = 0; i < size; ++i) {
        assert(p_con->monomials->at(i).is_linear);

        int var_idx = p_con->monomials->at(i).m_vars[0];
        if (_cons_fixvar[c].count(var_idx))
          sum = *sum +
                *(*gen(_solver._cur_assignment[var_idx], _solver._cur_assignment[var_idx]) * p_con->monomials->at(i).coeff);
        else
          sum = *sum + *(*_ranges[var_idx] * p_con->monomials->at(i).coeff);
        prefix[i] = sum->copy();
      }

      // calculate suffix range
      sum = gen(0, 0);
      for (int i = size - 1; i >= 0; --i) {
        assert(p_con->monomials->at(i).is_linear);

        int var_idx = p_con->monomials->at(i).m_vars[0];
        if (_cons_fixvar[c].count(var_idx))
          sum = *sum +
                *(*gen(_solver._cur_assignment[var_idx], _solver._cur_assignment[var_idx]) * p_con->monomials->at(i).coeff);
        else
          sum = *sum + *(*_ranges[var_idx] * p_con->monomials->at(i).coeff);
        suffix[i] = sum->copy();
      }

      // calculate every variable's range in the constraint
      for (int i = 0; i < size; ++i) {
        int var_idx = p_con->monomials->at(i).m_vars[0];
        if (_cons_fixvar[c].count(var_idx)) continue;

        std::unique_ptr<range::range> domain = bound->copy();
        if (i > 0) domain = *domain - *prefix[i - 1];
        if (i + 1 < size) domain = *domain - *suffix[i + 1];
        domain = *domain * (1 / p_con->monomials->at(i).coeff);

        buffer_store(var_idx);
        range::range::status prev_status = _ranges[var_idx]->get_status();

        if (_ranges[var_idx]->copy()->intersect(*domain) == range::range::status::INVALID) {
          rollback();
          return {};
        }

        Float lower_delt = _ranges[var_idx]->lower(), upper_delt = _ranges[var_idx]->upper();
        range::range::status status = _ranges[var_idx]->intersect(*domain);
        lower_delt -= _ranges[var_idx]->lower(), upper_delt -= _ranges[var_idx]->upper();
        if (status == range::range::status::CONSTANT) {
          constant_push(que, var_idx, impact_sol, prev_status);
        } else {
          ls_impact(que, var_idx, impact_sol, impact_que);
        }
      }

      // update quadratic constraints
      if (!update_qcons(que, impact_sol, impact_que)) {
        rollback();
        return {};
      }
    }
  }

  clear_buffers();
  return que;
}

std::vector<int> qp_solver::propagation::propagate_quadratic(std::vector<std::pair<int, Float>> &changes,
                                                             bool impact_sol, bool impact_que, const uint32_t access) {
  assert(access >> (uint32_t)_type & 1);
  if (impact_sol) clear_assignment();
  std::vector<int> que;
  for (int i = 0; i < changes.size(); ++i) {
    int idx = changes[i].first;
    Float val = changes[i].second;
    assert(0 <= idx && idx < _solver._vars.size());

    buffer_store(idx);
    range::range::status prev_status = _ranges[idx]->get_status();
    if (_ranges[idx]->copy()->intersect(*gen(val, val)) == range::range::status::INVALID) {
#ifdef PROP_LOG
      fprintf(stderr, "***[propagate_quadratic]*** %s changes invalid range\n", _solver._vars[idx].name.c_str());
      fprintf(stderr, "***[propagate_quadratic]*** rollback\n");
#endif
      rollback();
      return {};
    }
    _ranges[idx]->intersect(*gen(val, val));
    constant_push(que, idx, impact_sol, prev_status);
  }

  for (int qindex = 0; qindex < que.size(); ++qindex) {
    var *p_var = &_solver._vars[que[qindex]];
    for (const int qc : *p_var->constraints) {
      polynomial_constraint *p_con = &(_solver._constraints[qc]);

      // initialize bound and sum
      bool all_fixed = true;
      std::unique_ptr<range::range> bound, sum;
      Float lower = -std::numeric_limits<Float>::infinity(), upper = std::numeric_limits<Float>::infinity();
      if (p_con->is_equal)
        lower = upper = p_con->bound;
      else {
        if (p_con->is_less)
          upper = p_con->bound;
        else
          lower = p_con->bound;
      }
      sum = gen(0, 0);
      bound = gen(lower, upper);

      // check if the constraint is fixed
      size_t size = p_con->monomials->size();
      for (size_t i = 0; i < size && all_fixed; ++i) {
        int var_idx = p_con->monomials->at(i).m_vars[0];
        all_fixed &= _is_fixed[var_idx];
      }

      // check if the constraint is valid
      if (all_fixed) {
        Float value = 0;
        for (int i = 0; i < size; ++i) {
          monomial &m = p_con->monomials->at(i);
          if (m.m_vars.size() == 2) {
            int var_idx_1 = m.m_vars[0], var_idx_2 = m.m_vars[1];
            Float coeff = m.coeff;
            value += _ranges[var_idx_1]->lower() * _ranges[var_idx_2]->lower() * coeff;
          } else if (m.is_linear) {
            int var_idx = m.m_vars[0];
            Float coeff = m.coeff;
            value += _ranges[var_idx]->lower() * coeff;
          } else {
            int var_idx = m.m_vars[0];
            Float coeff = m.coeff;
            value += _ranges[var_idx]->lower() * _ranges[var_idx]->lower() * coeff;
          }
        }
        if (!bound->in_range(value)) {
#ifdef PROP_LOG
          fprintf(stderr, "***[propagate_quadratic]*** constraint %s is invalid\n", p_con->name.c_str());
          fprintf(stderr, "***[propagate_quadratic]*** bound: ");
          for (auto &p : bound->get_range()) {
            fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
          }
          fprintf(stderr, "\n");
          fprintf(stderr, "***[propagate_quadratic]*** value: %lf\n", value);
          fprintf(stderr, "***[propagate_quadratic]*** rollback\n");
#endif
          rollback();
          return {};
        }
        continue;
      }

#ifdef PROP_LOG
      fprintf(stderr, "***[propagate_quadratic]*** now at constraint %s\n", p_con->name.c_str());
#endif

      // calculate every variable's range in the constraint
      for (const auto &[var_idx, coeff] : *p_con->var_coeff) {
        std::unique_ptr<range::range> b = gen(coeff.obj_constant_coeff, coeff.obj_constant_coeff),
                                      c = *gen(0, 0) - *bound;
        for (int i = 0; i < coeff.obj_linear_coeff.size(); ++i) {
          int new_var = coeff.obj_linear_coeff[i];
          Float var_coeff = coeff.obj_linear_constant_coeff[i];
          b = *b + *(*_ranges[new_var] * var_coeff);
        }
        // [TODO] speed up
        for (auto &m : *p_con->monomials) {
          if (m.m_vars.size() == 2) {
            int m_var_idx_1 = m.m_vars[0], m_var_idx_2 = m.m_vars[1];
            Float coeff = m.coeff;
            if (m_var_idx_1 != var_idx && m_var_idx_2 != var_idx)
              c = *c + *(*(*_ranges[m_var_idx_1] * *_ranges[m_var_idx_2]) * coeff);
          } else if (m.is_linear) {
            int m_var_idx = m.m_vars[0];
            Float coeff = m.coeff;
            if (m_var_idx != var_idx) c = *c + *(*_ranges[m_var_idx] * coeff);
          } else {
            int m_var_idx = m.m_vars[0];
            Float coeff = m.coeff;
            if (m_var_idx != var_idx) c = *c + *(*_ranges[m_var_idx]->square() * coeff);
          }

#ifdef PROP_LOG
          fprintf(stderr, "***[propagate_quadratic]*** now calculate monomials\n");
          fprintf(stderr, "***[propagate_quadratic]*** a: %lf\n", coeff.obj_quadratic_coeff);
          fprintf(stderr, "***[propagate_quadratic]*** b: ");
          for (auto &p : b->get_range()) {
            fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
          }
          fprintf(stderr, "\n");
          fprintf(stderr, "***[propagate_quadratic]*** c: ");
          for (auto &p : c->get_range()) {
            fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
          }
          fprintf(stderr, "\n");
#endif
        }

#ifdef PROP_LOG
        fprintf(stderr, "***[propagate_quadratic]*** now at variable %s\n", _solver._vars[var_idx].name.c_str());
        fprintf(stderr, "***[propagate_quadratic]*** a: %lf\n", coeff.obj_quadratic_coeff);
        fprintf(stderr, "***[propagate_quadratic]*** b: ");
        for (auto &p : b->get_range()) {
          fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "***[propagate_quadratic]*** c: ");
        for (auto &p : c->get_range()) {
          fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
        }
        fprintf(stderr, "\n");
#endif

        std::unique_ptr<range::range> domain = solve_quadratic(coeff.obj_quadratic_coeff, *b, *c);
        buffer_store(var_idx);
        range::range::status prev_status = _ranges[var_idx]->get_status();
#ifdef PROP_LOG
        auto temp = _ranges[var_idx]->copy();
        temp->intersect(*domain);
        fprintf(stderr,
                "***[propagate_quadratic]*** after intersect variable %s range: ", _solver._vars[var_idx].name.c_str());
        for (auto &p : temp->get_range()) {
          fprintf(stderr, "[%lf, %lf] ", p.first, p.second);
        }
        fprintf(stderr, "\n");
#endif

        if (_ranges[var_idx]->copy()->intersect(*domain) == range::range::status::INVALID) {
#ifdef PROP_LOG
          fprintf(stderr, "***[propagate_quadratic]*** constraint %s changes invalid %s's range\n", p_con->name.c_str(),
                  _solver._vars[var_idx].name.c_str());
          fprintf(stderr, "***[propagate_quadratic]*** rollback\n");
#endif
          rollback();
          return {};
        }

        range::range::status status = _ranges[var_idx]->intersect(*domain);
        if (status == range::range::status::CONSTANT) {
          constant_push(que, var_idx, impact_sol, prev_status);
        } else {
          ls_impact(que, var_idx, impact_sol, impact_que);
        }
      }

#ifdef PROP_LOG
      fprintf(stderr, "***[propagate_quadratic]*** finish constraint %s\n", p_con->name.c_str());
#endif
    }
  }

  clear_buffers();
  return que;
}

}  // namespace solver