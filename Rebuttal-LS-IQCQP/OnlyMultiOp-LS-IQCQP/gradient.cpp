#include <cmath>
#include <limits>
#include <sstream>
#ifdef GRADIENT_DEBUG
#include <iostream>
#endif

#include "sol.h"

namespace solver {
#ifdef GRADIENT_DEBUG
static void debug_print_pool_size(const char* tag, size_t n) {
  std::cout << "[GRAD] " << tag << " pool size=" << n << std::endl;
}
#endif

#ifdef GRADIENT_DEBUG
static inline std::string var_name(const qp_solver* s, int idx) {
  if (idx >= 0 && idx < (int)s->_vars.size() && !s->_vars[idx].name.empty()) return s->_vars[idx].name;
  std::ostringstream oss;
  oss << "x" << idx;
  return oss.str();
}

static std::string format_expression(const qp_solver* s, const std::vector<monomial>& monos) {
  std::ostringstream oss;
  bool first = true;
  for (const auto& m : monos) {
    const long double coeff = m.coeff;
    if (coeff == 0.0L) continue;
    std::ostringstream term;
    // Build variable product
    if (m.is_multilinear && m.m_vars.size() == 2) {
      term << var_name(s, m.m_vars[0]) << " * " << var_name(s, m.m_vars[1]);
    } else if (m.is_linear && m.m_vars.size() == 1) {
      term << var_name(s, m.m_vars[0]);
    } else if (m.m_vars.size() == 1) {
      term << var_name(s, m.m_vars[0]) << "^2";
    } else {
      // Fallback for unexpected structures
      for (size_t k = 0; k < m.m_vars.size(); ++k) {
        if (k) term << " * ";
        term << var_name(s, m.m_vars[k]);
      }
    }

    auto abs_coeff = coeff < 0 ? -coeff : coeff;
    std::ostringstream piece;
    if (term.str().empty()) {
      // pure constant (rare in current design)
      piece << abs_coeff;
    } else {
      if (abs_coeff == 1.0L)
        piece << term.str();
      else
        piece << abs_coeff << " * " << term.str();
    }

    if (first) {
      if (coeff < 0)
        oss << "- " << piece.str();
      else
        oss << piece.str();
      first = false;
    } else {
      if (coeff < 0)
        oss << " - " << piece.str();
      else
        oss << " + " << piece.str();
    }
  }
  if (first) return std::string("0");
  return oss.str();
}

static void debug_print_constraint(const qp_solver* s, const polynomial_constraint& con) {
  const std::string expr = format_expression(s, con.monomials);
  const char* op = con.is_equal ? "==" : (con.is_less ? "<=" : ">=");
  std::cout << "[GRAD] cons: " << expr << " " << op << " " << con.bound << std::endl;
}

static void debug_print_objective(const qp_solver* s) {
  const std::string expr = format_expression(s, s->_object_monoials);
  std::cout << "[GRAD] obj: " << expr << std::endl;
}
#endif

// Common helper: build one gradient_operation from direction grad_g and step t, push into gradient_pool
void qp_solver::add_gradient_operation(const unordered_map<int, Float>& grad_g, int constraint_index, Float t) {
  gradient_operation op;
  op.constraint_index = constraint_index;
  op.variable_indices.reserve(grad_g.size());
  op.variable_deltas.reserve(grad_g.size());
  for (const auto& kv : grad_g) {
    const int v = kv.first;
    const Float g = kv.second;
    op.variable_indices.push_back(v);
    if (_vars[v].is_bin) {
      const Float x = _cur_assignment[v];
      Float new_val = (x + g * t >= 0.5L) ? 1.0L : 0.0L;
      if (_vars[v].has_lower) new_val = std::max(new_val, (Float)std::ceil(_vars[v].lower));
      if (_vars[v].has_upper) new_val = std::min(new_val, (Float)std::floor(_vars[v].upper));
      op.variable_deltas.push_back(new_val - x);
    } else if (_vars[v].is_int) {
      const Float x = _cur_assignment[v];
      Float new_val = std::round(x + g * t);
      if (_vars[v].has_lower) new_val = std::max(new_val, (Float)std::ceil(_vars[v].lower));
      if (_vars[v].has_upper) new_val = std::min(new_val, (Float)std::floor(_vars[v].upper));
      op.variable_deltas.push_back(new_val - x);
    } else {
      op.variable_deltas.push_back(g * t);
    }
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] candidate: t=" << t << ", vars=" << op.variable_indices.size() << std::endl;
  for (size_t i = 0; i < op.variable_indices.size(); ++i) {
    std::cout << "  v=" << op.variable_indices[i] << ", dv=" << op.variable_deltas[i] << std::endl;
  }
#endif
  bool any_change = false;
  for (const Float dv : op.variable_deltas) {
    if (fabsl(dv) > eb) {
      any_change = true;
      break;
    }
  }
  if (any_change)
    gradient_pool.push_back(std::move(op));
  else {
#ifdef GRADIENT_DEBUG
    std::cout << "[GRAD] skip zero-op (no variable changed)" << std::endl;
#endif
  }
}

// Compute objective gradient at current assignment
void qp_solver::compute_objective_gradient(unordered_map<int, Float>& out_grad) {
  out_grad.clear();
  // Similar to constraints: derivative of obj wrt each var
  for (size_t mono_idx = 0; mono_idx < _object_monoials.size(); ++mono_idx) {
    const monomial& mono = _object_monoials[mono_idx];
    if (mono.is_linear) {
      const int v = mono.m_vars[0];
      out_grad[v] += mono.coeff;
    } else if (mono.is_multilinear) {
      const int v1 = mono.m_vars[0];
      const int v2 = mono.m_vars[1];
      const Float x1 = _cur_assignment[v1];
      const Float x2 = _cur_assignment[v2];
      // d/dv1 (c * x1*x2) = c * x2; d/dv2 = c * x1
      out_grad[v1] += mono.coeff * x2;
      out_grad[v2] += mono.coeff * x1;
    } else {
      const int v = mono.m_vars[0];
      const Float x = _cur_assignment[v];
      // d/dv (c * x^2) = 2*c*x
      out_grad[v] += (2.0L * mono.coeff * x);
    }
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] obj gradient: ";
  bool first = true;
  for (const auto& kv : out_grad) {
    if (!first) std::cout << ", ";
    std::cout << var_name(this, kv.first) << ": " << kv.second;
    first = false;
  }
  if (first) std::cout << "(all zero)";
  std::cout << std::endl;
#endif
}

// Compute gradient of a constraint w.r.t. each continuous variable at current assignment
void qp_solver::compute_constraint_gradient(const polynomial_constraint& con, unordered_map<int, Float>& out_grad) {
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] compute_constraint_gradient begin" << std::endl;
  debug_print_constraint(this, con);
#endif
  out_grad.clear();
  for (const auto& kv : con.var_coeff) {
    const int var_idx = kv.first;
    ASSERT(!_vars[var_idx].is_constant);

    const all_coeff& a = kv.second;
    // linear part
    Float li = a.obj_constant_coeff;
    for (size_t p = 0; p < a.obj_linear_coeff.size(); ++p) {
      const int j = a.obj_linear_coeff[p];
      const Float lij = a.obj_linear_constant_coeff[p];
      li += _cur_assignment[j] * lij;
    }
    // quadratic part derivative 2*q*x
    const Float xi = _cur_assignment[var_idx];
    const Float qi = a.obj_quadratic_coeff;
    const Float gi = li + (2.0L * qi * xi);
    if (gi != 0) out_grad[var_idx] = gi;
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] gradient: ";
  bool first = true;
  for (const auto& gkv : out_grad) {
    if (!first) std::cout << ", ";
    std::cout << var_name(this, gkv.first) << ": " << gkv.second;
    first = false;
  }
  if (first) std::cout << "(all zero)";
  std::cout << std::endl;
#endif
}

// ------------------------------------------------------------

// Build q(t) = q0 + B t + A t^2 for obj along x <- x + g t
void qp_solver::construct_objective_parametric_polynomial(const unordered_map<int, Float>& grad_g, Float& A, Float& B,
                                                          Float& q0) {
  q0 = eval_objective_with_deltas({});
  A = 0;
  B = 0;
  auto get_val = [&](int var_idx) -> Float { return _cur_assignment[var_idx]; };
  auto get_g = [&](int var_idx) -> Float {
    auto it = grad_g.find(var_idx);
    return it == grad_g.end() ? 0.0L : it->second;
  };
  for (const auto& mono : _object_monoials) {
    if (mono.is_linear) {
      const int v = mono.m_vars[0];
      const Float gv = get_g(v);
      B += mono.coeff * gv;
    } else if (mono.is_multilinear) {
      const int v1 = mono.m_vars[0];
      const int v2 = mono.m_vars[1];
      const Float x1 = get_val(v1), x2 = get_val(v2);
      const Float g1 = get_g(v1), g2 = get_g(v2);
      B += mono.coeff * (x1 * g2 + x2 * g1);
      A += mono.coeff * (g1 * g2);
    } else {
      const int v = mono.m_vars[0];
      const Float x = get_val(v);
      const Float g = get_g(v);
      B += mono.coeff * (2.0L * x * g);
      A += mono.coeff * (g * g);
    }
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] q(t) = " << q0 << " + (" << B << ") * t + (" << A << ") * t^2" << std::endl;
#endif
}

// Build p(t) = p0 + B t + A t^2 for a constraint under x <- x + g t
void qp_solver::construct_constraint_parametric_polynomial(const polynomial_constraint& con,
                                                           const unordered_map<int, Float>& grad_g, Float& A, Float& B,
                                                           Float& p0) {
  p0 = con.value;
  A = 0;
  B = 0;
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] construct_constraint_parametric_polynomial begin" << std::endl;
  debug_print_constraint(this, con);
#endif
  auto get_val = [&](int var_idx) -> Float { return _cur_assignment[var_idx]; };
  auto get_g = [&](int var_idx) -> Float {
    auto it = grad_g.find(var_idx);
    if (it == grad_g.end()) return 0.0L;
    return it->second;
  };
  for (const auto& mono : con.monomials) {
    if (mono.is_linear) {
      const int v = mono.m_vars[0];
      const Float gv = get_g(v);
      B += mono.coeff * gv;
    } else {
      if (mono.is_multilinear) {
        const int v1 = mono.m_vars[0];
        const int v2 = mono.m_vars[1];
        const Float x1 = get_val(v1), x2 = get_val(v2);
        const Float g1 = get_g(v1), g2 = get_g(v2);
        B += mono.coeff * (x1 * g2 + x2 * g1);
        A += mono.coeff * (g1 * g2);
      } else {
        const int v = mono.m_vars[0];
        const Float x = get_val(v);
        const Float g = get_g(v);
        B += mono.coeff * (2.0L * x * g);
        A += mono.coeff * (g * g);
      }
    }
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] p(t) = " << p0 << " + (" << B << ") * t + (" << A << ") * t^2" << std::endl;
#endif
}

// Intersect variable bounds to obtain feasible t interval for x + g t
bool qp_solver::feasible_t_interval(const unordered_map<int, Float>& grad_g, Float& t_min, Float& t_max) {
  t_min = -std::numeric_limits<double>::infinity();
  t_max = std::numeric_limits<double>::infinity();
  for (const auto& kv : grad_g) {
    const int idx = kv.first;
    const Float g = kv.second;
    if (_vars[idx].is_bin) continue;
    if (g == 0) continue;
    const Float x = _cur_assignment[idx];
#ifdef GRADIENT_DEBUG
    std::cout << "[GRAD] feasible_t_interval var idx=" << idx << ", g=" << g << ", x=" << x << std::endl;
#endif
    if (_vars[idx].has_lower) {
      const Float bound_t = (_vars[idx].lower - x) / g;
      if (g > 0)
        t_min = std::max(t_min, bound_t);
      else
        t_max = std::min(t_max, bound_t);
    }
    if (_vars[idx].has_upper) {
      const Float bound_t = (_vars[idx].upper - x) / g;
      if (g > 0)
        t_max = std::min(t_max, bound_t);
      else
        t_min = std::max(t_min, bound_t);
    }
    if (t_min > t_max) return false;
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] feasible_t_interval: t_min=" << t_min << ", t_max=" << t_max << std::endl;
#endif
  return true;
}

// Build gradient-unsat ops for a given unsatisfied constraint
void qp_solver::gradient_build_unsat_pool() {
  gradient_pool.clear();
  for (int con_idx : _unsat_constraints) {
    const polynomial_constraint& con = _constraints[con_idx];
#ifdef GRADIENT_DEBUG
    std::cout << "[GRAD] build unsat ops: con_idx=" << con_idx << std::endl;
    debug_print_constraint(this, con);
#endif
    unordered_map<int, Float> grad_g;
    compute_constraint_gradient(con, grad_g);
    if (grad_g.empty()) continue;
    Float A = 0, B = 0, p0 = con.value;
    construct_constraint_parametric_polynomial(con, grad_g, A, B, p0);
    const Float C = p0 - con.bound;
    Float t_min, t_max;
    if (!feasible_t_interval(grad_g, t_min, t_max)) continue;
    auto clamp_t = [&](Float t) -> Float {
      if (t < t_min) return t_min;
      if (t > t_max) return t_max;
      return t;
    };

    std::vector<Float> candidate_ts;
    if (fabsl(A) < eb) {
      if (fabsl(B) >= eb) {
        Float t = clamp_t(-C / (B == 0 ? 1 : B));
        candidate_ts.push_back(t);
      }
    } else {
      double r0 = 0, r1 = 0;
      int roots = gsl_poly_solve_quadratic((double)A, (double)B, (double)C, &r0, &r1);
      if (roots == 1) {
        candidate_ts.push_back(clamp_t((Float)r0));
      } else if (roots == 2) {
        candidate_ts.push_back(clamp_t((Float)std::min(r0, r1)));
        candidate_ts.push_back(clamp_t((Float)std::max(r0, r1)));
      }
    }
    if (candidate_ts.empty() && B > 0) {
      Float t = clamp_t(std::min((Float)1.0L, std::max((Float)-1.0L, (Float)(-C / (B + 1e-18L)))));
      candidate_ts.push_back(t);
    }

    for (Float t : candidate_ts) {
      const Float pt = p0 + B * t + A * t * t;
      bool accept = false;
      if (con.is_equal)
        accept = (fabsl(pt - con.bound) <= eb);
      else if (con.is_less)
        accept = (pt <= con.bound + eb);
      else
        accept = (pt >= con.bound - eb);
      if (!accept) continue;
      add_gradient_operation(grad_g, con_idx, t);
    }
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] gradient_build_unsat_pool size=" << gradient_pool.size() << std::endl;
#endif
}

// Build SAT pool with two operators described:
// 1) objective gradient direction; choose t to minimize objective under all cons feasibility
// 2) for each constraint, use its gradient direction; pick t that minimizes objective while keeping that constraint
// satisfied
void qp_solver::gradient_build_sat_pool() {
  gradient_pool.clear();
  // Operator 1: objective gradient
  {
    unordered_map<int, Float> gobj;
    compute_objective_gradient(gobj);
    if (!gobj.empty()) {
      Float t_min, t_max;
      if (feasible_t_interval(gobj, t_min, t_max)) {
        Float A = 0, B = 0, q0 = 0;
        construct_objective_parametric_polynomial(gobj, A, B, q0);
        // minimize q(t) on [t_min, t_max]
        std::vector<Float> cand;
        if (fabsl(A) < eb) {
          if (fabsl(B) >= eb) cand.push_back(B > 0 ? t_min : t_max);
        } else {
          Float t_star = -B / (2 * A);
          if (t_star >= t_min && t_star <= t_max) cand.push_back(t_star);
          cand.push_back(t_min);
          cand.push_back(t_max);
        }
        for (Float t : cand) {
          // op1 requires t satisfy ALL constraints
          bool all_cons_ok = true;
          for (const auto& c : _constraints) {
            Float Ac = 0, Bc = 0, p0c = 0;
            construct_constraint_parametric_polynomial(c, gobj, Ac, Bc, p0c);
            const Float pt = p0c + Bc * t + Ac * t * t;
            bool ok;
            if (c.is_equal)
              ok = (fabsl(pt - c.bound) <= eb);
            else if (c.is_less)
              ok = (pt <= c.bound + eb);
            else
              ok = (pt >= c.bound - eb);
            if (!ok) {
              all_cons_ok = false;
              break;
            }
          }
          if (!all_cons_ok) continue;
          add_gradient_operation(gobj, -1, t);
        }
      }
    }
  }
  // Operator 2: each constraint's gradient direction
  for (size_t ci = 0; ci < _constraints.size(); ++ci) {
    const polynomial_constraint& con = _constraints[ci];
    unordered_map<int, Float> gcon;
    compute_constraint_gradient(con, gcon);
    if (gcon.empty()) continue;
    Float t_min, t_max;
    if (!feasible_t_interval(gcon, t_min, t_max)) continue;
    // choose t that satisfies this constraint, similar to gradient_build_unsat_pool
    Float Ac = 0, Bc = 0, p0c = 0;
    construct_constraint_parametric_polynomial(con, gcon, Ac, Bc, p0c);
    const Float Cc = p0c - con.bound;
    auto clamp_t = [&](Float t) -> Float {
      if (t < t_min) return t_min;
      if (t > t_max) return t_max;
      return t;
    };
    std::vector<Float> cand;
    if (fabsl(Ac) < eb) {
      if (fabsl(Bc) >= eb) {
        Float t = clamp_t(-Cc / (Bc == 0 ? 1 : Bc));
        cand.push_back(t);
      }
    } else {
      double r0 = 0, r1 = 0;
      int roots = gsl_poly_solve_quadratic((double)Ac, (double)Bc, (double)Cc, &r0, &r1);
      if (roots == 1) {
        cand.push_back(clamp_t((Float)r0));
      } else if (roots == 2) {
        cand.push_back(clamp_t((Float)std::min(r0, r1)));
        cand.push_back(clamp_t((Float)std::max(r0, r1)));
      }
    }
    // Also try objective symmetry axis t* = -B_obj / (2*A_obj) along this direction
    {
      Float Aobj = 0, Bobj = 0, q0tmp = 0;
      construct_objective_parametric_polynomial(gcon, Aobj, Bobj, q0tmp);
      if (fabsl(Aobj) >= eb) {
        Float t_star = -Bobj / (2 * Aobj);
        if (t_star >= t_min && t_star <= t_max) cand.push_back(t_star);
      }
    }
    if (cand.empty() && Bc > 0) {
      Float t = clamp_t(std::min((Float)1.0L, std::max((Float)-1.0L, (Float)(-Cc / (Bc + 1e-18L)))));
      cand.push_back(t);
    }
    for (Float t : cand) {
      const Float pt = p0c + Bc * t + Ac * t * t;
      bool accept = false;
      if (con.is_equal)
        accept = (fabsl(pt - con.bound) <= eb);
      else if (con.is_less)
        accept = (pt <= con.bound + eb);
      else
        accept = (pt >= con.bound - eb);
      if (!accept) continue;
      add_gradient_operation(gcon, (int)ci, t);
    }
  }
#ifdef GRADIENT_DEBUG
  debug_print_pool_size("sat", gradient_pool.size());
#endif
}

// ------------------------------------------------------------

// Evaluate constraint value with hypothetical new values for a subset of variables
Float qp_solver::eval_constraint_with_deltas(const polynomial_constraint& con,
                                             const unordered_map<int, Float>& idx_to_new_value) {
  auto get_val = [&](int var_idx) -> Float {
    auto it = idx_to_new_value.find(var_idx);
    if (it != idx_to_new_value.end()) return it->second;
    return _cur_assignment[var_idx];
  };
  Float sum = 0;
  for (const auto& mono : con.monomials) {
    if (mono.is_linear) {
      const int v = mono.m_vars[0];
      sum += mono.coeff * get_val(v);
    } else {
      if (mono.is_multilinear) {
        const int v1 = mono.m_vars[0];
        const int v2 = mono.m_vars[1];
        sum += mono.coeff * get_val(v1) * get_val(v2);
      } else {
        const int v = mono.m_vars[0];
        const Float xv = get_val(v);
        sum += mono.coeff * xv * xv;
      }
    }
  }
  return sum;
}

// Evaluate objective value with hypothetical new values for a subset of variables
Float qp_solver::eval_objective_with_deltas(const unordered_map<int, Float>& idx_to_new_value) {
  auto get_val = [&](int var_idx) -> Float {
    auto it = idx_to_new_value.find(var_idx);
    if (it != idx_to_new_value.end()) return it->second;
    return _cur_assignment[var_idx];
  };
  Float sum = _obj_constant;
  for (const auto& mono : _object_monoials) {
    if (mono.is_linear) {
      const int v = mono.m_vars[0];
      sum += mono.coeff * get_val(v);
    } else {
      if (mono.is_multilinear) {
        const int v1 = mono.m_vars[0];
        const int v2 = mono.m_vars[1];
        sum += mono.coeff * get_val(v1) * get_val(v2);
      } else {
        const int v = mono.m_vars[0];
        const Float xv = get_val(v);
        sum += mono.coeff * xv * xv;
      }
    }
  }
  return sum;
}

// ------------------------------------------------------------

Float qp_solver::calculate_score_multi_bin_cy(const unordered_map<int, Float>& new_vals) {
  Float score = 0;
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] calculate_score_multi_bin_cy begin" << std::endl;
  debug_print_objective(this);
#endif
  for (auto& con_ref : _constraints) {
    polynomial_constraint* pcon = const_cast<polynomial_constraint*>(&con_ref);
#ifdef GRADIENT_DEBUG
    debug_print_constraint(this, con_ref);
#endif
    const Float new_val = eval_constraint_with_deltas(con_ref, new_vals);
    const Float var_delta_total = new_val - con_ref.value;
    const Float state = judge_cons_state_bin_cy(pcon, var_delta_total, new_val);
    score += (con_ref.weight * state);
#ifdef GRADIENT_DEBUG
    std::cout << "  cons idx=" << con_ref.index << ", old_val=" << con_ref.value << ", new_val=" << new_val
              << ", delta=" << var_delta_total << ", state=" << state << ", weighted_add=" << (con_ref.weight * state)
              << std::endl;
#endif
  }

  const Float old_obj = eval_objective_with_deltas({});
  const Float new_obj = eval_objective_with_deltas(new_vals);
  const Float obj_delta = new_obj - old_obj;
  score += -_object_weight * obj_delta;
#ifdef GRADIENT_DEBUG
  std::cout << "  objective old=" << old_obj << ", new=" << new_obj << ", delta=" << obj_delta
            << ", contrib=" << (-_object_weight * obj_delta) << std::endl;
  std::cout << "[GRAD] calculate_score_multi_bin_cy end: score=" << score << std::endl;
#endif

  return score;
}

Float qp_solver::calculate_score_multi_bin(const unordered_map<int, Float>& new_vals) {
  Float score = 0;
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] calculate_score_multi_bin begin" << std::endl;
#endif
  for (auto& con_ref : _constraints) {
    polynomial_constraint* pcon = const_cast<polynomial_constraint*>(&con_ref);
#ifdef GRADIENT_DEBUG
    debug_print_constraint(this, con_ref);
#endif
    const Float new_val = eval_constraint_with_deltas(con_ref, new_vals);
    const Float var_delta_total = new_val - con_ref.value;
    const Float state = judge_cons_state_bin(pcon, var_delta_total, new_val);
    score += (con_ref.weight * state);
#ifdef GRADIENT_DEBUG
    std::cout << "  cons idx=" << con_ref.index << ", old_val=" << con_ref.value << ", new_val=" << new_val
              << ", delta=" << var_delta_total << ", state=" << state << ", weighted_add=" << (con_ref.weight * state)
              << std::endl;
#endif
  }

#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] calculate_score_multi_bin end: score=" << score << std::endl;
#endif
  return score;
}

// Multi-variable score (mix) using continuous distance-based evaluation (cy variant)
Float qp_solver::calculate_score_multi_mix_cy(const unordered_map<int, Float>& new_vals) {
  Float score = 0;
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] calculate_score_multi_mix_cy begin" << std::endl;
#endif
  for (auto& con_ref : _constraints) {
    polynomial_constraint* pcon = const_cast<polynomial_constraint*>(&con_ref);
#ifdef GRADIENT_DEBUG
    debug_print_constraint(this, con_ref);
#endif
    const Float new_val = eval_constraint_with_deltas(con_ref, new_vals);
    const Float var_delta_total = new_val - con_ref.value;
    const Float state = judge_cons_state_bin_cy_mix(pcon, var_delta_total, new_val);
    score += (con_ref.weight * state);
#ifdef GRADIENT_DEBUG
    std::cout << "  cons idx=" << con_ref.index << ", old_val=" << con_ref.value << ", new_val=" << new_val
              << ", delta=" << var_delta_total << ", state=" << state << ", weighted_add=" << (con_ref.weight * state)
              << std::endl;
#endif
  }

  const Float old_obj = eval_objective_with_deltas({});
  const Float new_obj = eval_objective_with_deltas(new_vals);
  const Float obj_delta = new_obj - old_obj;
  score += -_object_weight * obj_delta;
#ifdef GRADIENT_DEBUG
  std::cout << "  objective old=" << old_obj << ", new=" << new_obj << ", delta=" << obj_delta
            << ", contrib=" << (-_object_weight * obj_delta) << std::endl;
  std::cout << "[GRAD] calculate_score_multi_mix_cy end: score=" << score << std::endl;
#endif
  return score;
}

// Multi-variable score (mix) using discrete +/- style state
Float qp_solver::calculate_score_multi_mix(const unordered_map<int, Float>& new_vals) {
  Float score = 0;
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] calculate_score_multi_mix begin" << std::endl;
#endif
  for (auto& con_ref : _constraints) {
    polynomial_constraint* pcon = const_cast<polynomial_constraint*>(&con_ref);
#ifdef GRADIENT_DEBUG
    debug_print_constraint(this, con_ref);
#endif
    const Float new_val = eval_constraint_with_deltas(con_ref, new_vals);
    const Float var_delta_total = new_val - con_ref.value;
    const Float state = judge_cons_state_mix(pcon, var_delta_total, new_val);
    score += (con_ref.weight * state);
#ifdef GRADIENT_DEBUG
    std::cout << "  cons idx=" << con_ref.index << ", old_val=" << con_ref.value << ", new_val=" << new_val
              << ", delta=" << var_delta_total << ", state=" << state << ", weighted_add=" << (con_ref.weight * state)
              << std::endl;
#endif
  }

  const Float old_obj = eval_objective_with_deltas({});
  const Float new_obj = eval_objective_with_deltas(new_vals);
  const Float obj_delta = new_obj - old_obj;
  score += -_object_weight * obj_delta;
#ifdef GRADIENT_DEBUG
  std::cout << "  objective old=" << old_obj << ", new=" << new_obj << ", delta=" << obj_delta
            << ", contrib=" << (-_object_weight * obj_delta) << std::endl;
  std::cout << "[GRAD] calculate_score_multi_mix end: score=" << score << std::endl;
#endif
  return score;
}

// Score for multi-variable move
Float qp_solver::compute_multivar_move_score_bin(const vector<int>& var_indices, const vector<Float>& var_deltas) {
  ASSERT(var_indices.size() == var_deltas.size());
  // Build new value map for variables affected by the move
  unordered_map<int, Float> new_vals;
  new_vals.reserve(var_indices.size());
  for (size_t i = 0; i < var_indices.size(); ++i) {
    const int vid = var_indices[i];
    new_vals[vid] = _cur_assignment[vid] + var_deltas[i];
  }

  if (is_cur_feasible || cons_num_type == 0)
    return calculate_score_multi_bin_cy(new_vals);
  else
    return calculate_score_multi_bin(new_vals);
}

// Score for multi-variable move in mix setting
Float qp_solver::compute_multivar_move_score_mix(const vector<int>& var_indices, const vector<Float>& var_deltas) {
  ASSERT(var_indices.size() == var_deltas.size());
  // Build new value map for variables affected by the move
  unordered_map<int, Float> new_vals;
  new_vals.reserve(var_indices.size());
  for (size_t i = 0; i < var_indices.size(); ++i) {
    const int vid = var_indices[i];
    new_vals[vid] = _cur_assignment[vid] + var_deltas[i];
  }

  if (is_cur_feasible)
    return calculate_score_multi_mix_cy(new_vals);
  else
    return calculate_score_multi_mix(new_vals);
}

// BMS selection: score only a random subset of candidates to avoid full evaluation
bool qp_solver::gradient_select_best_op_bms(
    int& best_index, Float& best_score,
    const std::function<Float(const vector<int>&, const vector<Float>&)>& score_fn) {
  if (gradient_pool.empty()) return false;
  const int op_size = (int)gradient_pool.size();
  const int sample = std::min(bms, op_size);
  best_score = INT32_MIN;
  bool found = false;

  // Partial Fisher–Yates shuffle to sample without replacement
  std::vector<int> indices(op_size);
  for (int i = 0; i < op_size; ++i) indices[i] = i;
  for (int i = 0; i < sample; ++i) {
    int pick = i + (rand() % (op_size - i));
    std::swap(indices[i], indices[pick]);
  }

  for (int i = 0; i < sample; ++i) {
    const int idx = indices[i];
    const auto& op = gradient_pool[idx];
    const Float s = score_fn(op.variable_indices, op.variable_deltas);
#ifdef GRADIENT_DEBUG
    std::cout << "[GRAD] BMS sample idx=" << idx << ": score=" << s << std::endl;
#endif
    if (s > best_score) {
      best_score = s;
      best_index = idx;
      found = true;
    }
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] BMS best idx=" << (found ? best_index : -1) << ", score=" << (found ? best_score : 0)
            << std::endl;
#endif
  return found;
}

// ------------------------------------------------------------

// Execute a multi-variable gradient operation by index
void qp_solver::gradient_execute_op_index(int op_index, const std::function<void(int, Float)>& exec_fn) {
  const auto& op = gradient_pool[op_index];
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] execute grad op idx=" << op_index << ", vars=" << op.variable_indices.size() << std::endl;
  for (size_t i = 0; i < op.variable_indices.size(); ++i) {
    std::cout << "  v=" << op.variable_indices[i] << ", dv=" << op.variable_deltas[i] << std::endl;
  }
#endif
  for (size_t i = 0; i < op.variable_indices.size(); ++i) {
    const int v = op.variable_indices[i];
    const Float dv = op.variable_deltas[i];
    exec_fn(v, dv);
  }
}

// ------------------------------------------------------------

void qp_solver::gradient_random_walk_unsat_bin() {
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(
      best_idx, best_grad_score, [this](const vector<int>& var_indices, const vector<Float>& var_deltas) -> Float {
        return compute_multivar_move_score_bin(var_indices, var_deltas);
      });

  if (has_grad) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else {
    no_operation_walk_unsat();
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] random_walk_mix_not_dis choose: has_grad=" << has_grad << ", grad_score=" << best_grad_score
            << std::endl;
#endif
}
// Random walk for SAT (bin with equal), mixing classic operators and a gradient op candidate
void qp_solver::gradient_random_walk_sat_bin_with_equal() {
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(best_idx, best_grad_score,
                                              [this](const vector<int>& idx, const vector<Float>& dv) -> Float {
                                                return compute_multivar_move_score_bin(idx, dv);
                                              });

  if (has_grad) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else {
    // fallback no-op style
    int no_operation_var = rand() % _vars.size();
    Float value = (_cur_assignment[no_operation_var] == 0) ? 1 : -1;
    if (check_var_shift_bool(no_operation_var, value, 1)) execute_critical_move(no_operation_var, value);
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] sat random walk choose: has_grad=" << has_grad << ", grad_score=" << best_grad_score
            << std::endl;
#endif
}

void qp_solver::gradient_random_walk_sat_bin() {
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(best_idx, best_grad_score,
                                              [this](const vector<int>& idx, const vector<Float>& dv) -> Float {
                                                return compute_multivar_move_score_bin(idx, dv);
                                              });

  if (has_grad) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else {
#ifdef DEBUG
    cout << " sat random walk " << endl;
#endif
    int no_operation_var = rand() % _vars.size();
    Float value = (_cur_assignment[no_operation_var] == 0) ? 1 : -1;
    if (check_var_shift_bool(no_operation_var, value, 1)) execute_critical_move(no_operation_var, value);
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] sat random walk choose: has_grad=" << has_grad << ", grad_score=" << best_op_score
            << ", single_score=" << score << std::endl;
#endif
}

// Build candidates following random_walk_no_cons logic
void qp_solver::gradient_random_walk_no_cons() {
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(best_idx, best_grad_score,
                                              [this](const vector<int>& idx, const vector<Float>& dv) -> Float {
                                                return compute_multivar_move_score_bin(idx, dv);
                                              });

  if (has_grad) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_no_cons(v, dv); });
  } else {
    int rand_idx = rand() % _vars.size();
    Float value = (_cur_assignment[rand_idx] == 0) ? 1 : -1;
    if (check_var_shift_bool(rand_idx, value, 1)) execute_critical_move_no_cons(rand_idx, value);
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] no_cons random walk choose: has_grad=" << has_grad << ", grad_score=" << best_op_score
            << ", single_score=" << score << std::endl;
#endif
}

void qp_solver::gradient_random_walk_balance() {
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(best_idx, best_grad_score,
                                              [this](const vector<int>& idx, const vector<Float>& dv) -> Float {
                                                return compute_multivar_move_score_mix(idx, dv);
                                              });
  if (has_grad) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else {
    int no_operation_var = rand() % _vars.size();
    no_operation_walk_sat(no_operation_var);
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] random_walk_balance choose: has_grad=" << has_grad << ", grad_score=" << best_grad_score
            << std::endl;
#endif
}

void qp_solver::gradient_random_walk_unsat_mix_not_dis() {
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(
      best_idx, best_grad_score, [this](const vector<int>& var_indices, const vector<Float>& var_deltas) -> Float {
        return compute_multivar_move_score_mix(var_indices, var_deltas);
      });

  if (has_grad) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else {
    no_operation_walk_unsat();
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] random_walk_unsat_mix_not_dis choose: has_grad=" << has_grad << ", grad_score=" << best_op_score
            << ", single_score=" << score << std::endl;
#endif
}

// ------------------------------------------------------------

void qp_solver::gradient_bin_new_unsat_step() {
  // Build candidates first without computing real scores to avoid full evaluation cost
  gradient_build_unsat_pool();

  // BMS sampling: score sampled candidates and pick the best
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(
      best_idx, best_grad_score, [this](const vector<int>& var_indices, const vector<Float>& var_deltas) -> Float {
        return compute_multivar_move_score_bin(var_indices, var_deltas);
      });

  if (best_grad_score > 0) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else {
    update_weight();
    gradient_random_walk_unsat_bin();
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] step decision: var_pos=" << var_pos << ", single_score=" << score << ", has_grad=" << has_grad
            << ", grad_idx=" << (has_grad ? best_idx : -1) << ", grad_score=" << (has_grad ? best_grad_score : 0)
            << std::endl;
#endif
}

// SAT step analogous to gradient_bin_new_unsat_step but uses SAT pool
void qp_solver::gradient_bin_new_sat_step() {
  gradient_build_sat_pool();

  // BMS selection over gradient ops, scored with bin-style scorer (feasible context)
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(best_idx, best_grad_score,
                                              [this](const vector<int>& idx, const vector<Float>& dv) -> Float {
                                                return compute_multivar_move_score_bin(idx, dv);
                                              });

  if (best_grad_score > 0) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else {
    update_weight();
    gradient_random_walk_sat_bin_with_equal();
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] sat step decide: single(var=" << var_pos << ", score=" << score << "), pair(vars=("
            << var_pos_equal_1 << "," << var_pos_equal_2 << "), score=" << score_equal << "), grad(has=" << has_grad
            << ", idx=" << (has_grad ? best_idx : -1) << ", score=" << (has_grad ? best_grad_score : 0)
            << "), chosen_score=" << best_score << std::endl;
#endif
}

// SAT step variant for local_search_bin style (no equal-pair operator),
// analogous to local_search_bin's SAT phase but augmented with gradient SAT pool.
void qp_solver::gradient_bin_sat_step() {
  // build gradient SAT candidate pool
  gradient_build_sat_pool();

  // BMS selection over gradient ops, scored with bin-style scorer
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(best_idx, best_grad_score,
                                              [this](const vector<int>& idx, const vector<Float>& dv) -> Float {
                                                return compute_multivar_move_score_bin(idx, dv);
                                              });

  if (best_grad_score > 0) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else {
    update_weight();
    gradient_random_walk_sat_bin();
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] sat(bin) step decide: single(var=" << var_pos << ", score=" << score
            << "), grad(has=" << has_grad << ", idx=" << (has_grad ? best_idx : -1)
            << ", score=" << (has_grad ? best_grad_score : 0) << ")"
            << ", chosen_score=" << best_score << std::endl;
#endif
}

void qp_solver::gradient_no_cons_step() {
  // Build gradient candidate pool
  gradient_build_sat_pool();

  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(best_idx, best_grad_score,
                                              [this](const vector<int>& idx, const vector<Float>& dv) -> Float {
                                                return compute_multivar_move_score_bin(idx, dv);
                                              });

  if (has_grad) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_no_cons(v, dv); });
  } else {
    // No improvement: follow ls_no_cons fallback order: try fps_move; if it fails, update weights and random walk
    if (!fps_move()) {
      update_weight_no_cons();
      gradient_random_walk_no_cons();
    }
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] no_cons step decide: single(var=" << var_pos << ", score=" << score << "), grad(has=" << has_grad
            << ", idx=" << (has_grad ? best_idx : -1) << ", score=" << (has_grad ? best_grad_score : 0) << ")"
            << ", chosen_score=" << best_score << std::endl;
#endif
}

void qp_solver::gradient_mix_sat_step() {
  // Build gradient candidates for SAT phase
  gradient_build_sat_pool();

  // BMS: sample and score gradient candidates (reuse bin-style multi-var scoring; feasibility is handled inside)
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(best_idx, best_grad_score,
                                              [this](const vector<int>& idx, const vector<Float>& dv) -> Float {
                                                return compute_multivar_move_score_mix(idx, dv);
                                              });

  if (has_grad) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else {
    // No improvement: follow mix_balance fallback strategy
    update_weight();
    gradient_random_walk_balance();
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] mix sat step decide: single(var=" << var_pos << ", score=" << score << "), pair(vars=("
            << var_pos_equal_1 << "," << var_pos_equal_2 << "), score=" << score_equal << "), grad(has=" << has_grad
            << ", idx=" << (has_grad ? best_idx : -1) << ", score=" << (has_grad ? best_grad_score : 0)
            << "), chosen_score=" << best_score << std::endl;
#endif
}

void qp_solver::gradient_mix_unsat_step() {
  // Build UNSAT gradient candidates
  gradient_build_unsat_pool();

  // BMS: sample to pick the best gradient candidate
  int best_idx = -1;
  Float best_grad_score = INT32_MIN;
  bool has_grad = gradient_select_best_op_bms(
      best_idx, best_grad_score, [this](const vector<int>& var_indices, const vector<Float>& var_deltas) -> Float {
        return compute_multivar_move_score_mix(var_indices, var_deltas);
      });

  if (has_grad) {
    gradient_execute_op_index(best_idx, [this](int v, Float dv) { execute_critical_move_mix(v, dv); });
  } else if (!compensate_move()) {
    update_weight();
    // UNSAT random walk supports mixed gradient candidates
    gradient_random_walk_unsat_mix_not_dis();
  }
#ifdef GRADIENT_DEBUG
  std::cout << "[GRAD] mix unsat step decision: var_pos=" << var_pos << ", single_score=" << score
            << ", has_grad=" << has_grad << ", grad_idx=" << (has_grad ? best_idx : -1)
            << ", grad_score=" << (has_grad ? best_grad_score : 0) << std::endl;
#endif
}

}  // namespace solver