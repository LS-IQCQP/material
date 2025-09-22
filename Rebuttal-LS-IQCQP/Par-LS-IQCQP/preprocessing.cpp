#include <cassert>
#include <map>
#include <numeric>
#include <set>
#include <vector>

#include "propagation.h"
#include "sol.h"

namespace preprocessor {

constexpr Float eps = 1e-10;

struct union_set {
  std::vector<int> fa, size;

  union_set() = delete;
  union_set(int siz) {
    fa.resize(siz);
    size.resize(siz, 1);

    std::iota(fa.begin(), fa.end(), 0);
  }
  ~union_set() = default;

  int findset(int x) { return fa[x] == x ? x : (fa[x] = findset(fa[x])); }
  int getsize(int x) { return size[findset(x)]; }
  void merge(int x, int y) {
    int fx = findset(x), fy = findset(y);
    if (fx == fy) return;

    if (size[fx] < size[fy]) std::swap(fx, fy);

    fa[fy] = fx;
    size[fx] += size[fy];
  }
};

struct gauss {
  int n, m, rank;
  std::vector<std::vector<Float>> matrix;

  void rref() {
    n = matrix.size(), m = matrix[0].size(), rank = 0;

    for (int c = 0; c < m && rank < n; c++) {
      int ch = rank;
      for (int i = rank + 1; i < n; i++)
        if (std::abs(matrix[i][c]) > std::abs(matrix[ch][c])) ch = i;
      if (std::abs(matrix[ch][c]) < eps) continue;
      std::swap(matrix[ch], matrix[rank]);

      Float s = 1.0 / matrix[rank][c];
      for (int j = 0; j < m; j++) matrix[rank][j] *= s;
      for (int i = 0; i < n; i++)
        if (i != rank) {
          Float t = matrix[i][c];
          for (int j = 0; j < m; j++) matrix[i][j] -= t * matrix[rank][j];
        }
      rank++;
    }
  }
  void add_row(const std::vector<Float> &row) { matrix.push_back(row); }
  void clear() { matrix.clear(); }
};

}  // namespace preprocessor

namespace solver {

void qp_solver::union_vars() {
  preprocessor::union_set us(_vars.size());
  for (const auto &con : _constraints) {
    if (con.is_quadratic || !con.is_equal) continue;

    int prev = -1;
    for (const auto &mon : *con.monomials) {
      assert(mon.m_vars.size() == 1);

      int cur = mon.m_vars[0];
      if (prev == -1) {
        prev = cur;
      } else {
#ifdef UNION_LOG
        fprintf(stderr, "***[union_vars]*** merge %s and %s\n", _vars[prev].name.c_str(), _vars[cur].name.c_str());
#endif

        us.merge(prev, cur);
      }
    }
  }

  _union_label.resize(_vars.size(), -1);
  for (int i = 0; i < _vars.size(); i++) {
    int fa = us.findset(i);
    if (_union_label[fa] == -1) _union_label[fa] = _union_size++;
    _union_label[i] = _union_label[fa];
  }

  _union_cons.resize(_union_size);
  _union_vars.resize(_union_size);
  for (int i = 0; i < _vars.size(); i++) {
    _union_vars[_union_label[i]].push_back(i);
    for (const auto &con : *_vars[i].constraints) {
      if (_constraints[con].is_equal && !_constraints[con].is_quadratic) _union_cons[_union_label[i]].insert(con);
    }
  }
}

void qp_solver::calc_freevars(int idx) {
#ifdef PREPROCESSOR_LOG
  fprintf(stderr, "***[calc_freevars]*** now at union set %d\n", idx);
  fprintf(stderr, "***[calc_freevars]*** vars mapping: ");
  for (int i = 0; i < _union_vars[idx].size(); ++i)
    fprintf(stderr, "%s -> %d, ", _vars[_union_vars[idx][i]].name.c_str(), i);
  fprintf(stderr, "\n");
  fprintf(stderr, "***[calc_freevars]*** relevant constraints: ");
  for (const int &c : _union_cons[idx]) fprintf(stderr, "%s, ", _constraints[c].name.c_str());
  fprintf(stderr, "\n");
#endif

  if (_union_cons[idx].size() <= 1) return;

  std::sort(_union_vars[idx].begin(), _union_vars[idx].end(), [&](int a, int b) {
    int is_int_a = _vars[a].is_int, is_int_b = _vars[b].is_int, in_obj_a = _vars_in_obj.count(a),
        in_obj_b = _vars_in_obj.count(b);
    if (is_int_a != is_int_b) return is_int_a < is_int_b;
    if (in_obj_a != in_obj_b) return in_obj_a < in_obj_b;
    return a < b;
  });
  std::map<int, int> var_map;
  for (int i = 0; i < _union_vars[idx].size(); ++i) {
    int var_idx = _union_vars[idx][i];
    var_map[var_idx] = i;
  }

  preprocessor::gauss g;
  for (const int &c : _union_cons[idx]) {
    assert(_constraints[c].is_equal && !_constraints[c].is_quadratic);
    _removed_cons[c] = true;

    std::vector<Float> row(_union_vars[idx].size() + 1, 0);
    for (const auto &mon : *_constraints[c].monomials) {
      assert(mon.m_vars.size() == 1);
      int var = var_map[mon.m_vars[0]];
      row[var] = mon.coeff;
    }
    row[_union_vars[idx].size()] = _constraints[c].bound;
    g.add_row(row);
  }
  g.rref();
  if (10 * g.rank <= _union_vars[idx].size()) {
    for (const int &c : _union_cons[idx]) _removed_cons[c] = false;
    return;
  }

  for (int i = 0; i < g.rank; ++i) {
    int map_idx = -1;
    for (int j = 0; j < g.m; ++j) {
      if (std::abs(g.matrix[i][j]) > preprocessor::eps) {
        map_idx = j;
        break;
      }
    }
    assert(map_idx != -1);

    Float coeff = g.matrix[i][map_idx];
    int var_idx = _union_vars[idx][map_idx];
    _vars_formula[var_idx].clear();
    _vars_formula[var_idx].emplace_back(-1, g.matrix[i].back() / coeff);

    for (int j = map_idx + 1; j < g.m - 1; ++j) {
      if (std::abs(g.matrix[i][j]) > preprocessor::eps) {
        int var_idx2 = _union_vars[idx][j];
        _vars_formula[var_idx].emplace_back(var_idx2, -g.matrix[i][j] / coeff);
      }
    }

#ifdef PREPROCESSOR_LOG
    fprintf(stderr, "***[calc_freevars]*** nonfree var %s = ", _vars[var_idx].name.c_str());
    for (auto [idx, coeff] : _vars_formula[var_idx]) {
      if (idx == -1) {
        fprintf(stderr, "%0.15Lf, ", coeff);
      } else {
        fprintf(stderr, "%0.15Lf * %s, ", coeff, _vars[idx].name.c_str());
      }
    }
    fprintf(stderr, "\n");
#endif
  }
}

void qp_solver::add_constraint(qp_solver *solver, polynomial_constraint &con) {
  ++solver->_cons_num;
  con.index = solver->_cons_num;
  solver->_constraints.push_back(con);
  avg_bound += std::abs(con.bound);

  for (const auto &mon : *con.monomials) {
    for (const auto &var : mon.m_vars) {
      assert(var >= 0 && var < solver->_vars.size());
      solver->_vars[var].constraints->insert(con.index);
    }
  }
}

void qp_solver::add_monomials(polynomial_constraint &con, std::map<std::pair<int, int>, Float> &mono_map) {
  for (const auto &[key, coeff] : mono_map) {
    assert(coeff != 0);

    assert(key.first != -1 || key.second != -1);
    if (key.first == -1) {
      int var_idx = key.second;
      monomial mon(var_idx, coeff, true);
      con.monomials->push_back(mon);

      // build var's coeff
      auto coeff_pos = con.var_coeff->find(var_idx);
      if (coeff_pos != con.var_coeff->end())
        coeff_pos->second.obj_constant_coeff += coeff;
      else {
        all_coeff new_coeff;
        new_coeff.obj_constant_coeff += coeff;
        con.var_coeff->emplace(var_idx, new_coeff);
      }
    } else if (key.first == key.second) {
      int var_idx = key.second;
      monomial mon(var_idx, coeff, false);
      con.monomials->push_back(mon);

      // build var's coeff
      auto coeff_pos = con.var_coeff->find(var_idx);
      if (coeff_pos != con.var_coeff->end())
        coeff_pos->second.obj_quadratic_coeff += coeff;
      else {
        all_coeff new_coeff;
        new_coeff.obj_quadratic_coeff += coeff;
        con.var_coeff->emplace(var_idx, new_coeff);
      }
    } else {
      int var_idx1 = key.first, var_idx2 = key.second;
      monomial mon(var_idx1, var_idx2, coeff, false);
      con.monomials->push_back(mon);

      // build var's coeff
      auto coeff_pos_1 = con.var_coeff->find(var_idx1);
      auto coeff_pos_2 = con.var_coeff->find(var_idx2);
      if (coeff_pos_1 != con.var_coeff->end()) {
        coeff_pos_1->second.obj_linear_constant_coeff.push_back(coeff);
        coeff_pos_1->second.obj_linear_coeff.push_back(var_idx2);
      } else {
        all_coeff new_coeff;
        new_coeff.obj_linear_constant_coeff.push_back(coeff);
        new_coeff.obj_linear_coeff.push_back(var_idx2);
        con.var_coeff->emplace(var_idx1, new_coeff);
      }
      if (coeff_pos_2 != con.var_coeff->end()) {
        coeff_pos_2->second.obj_linear_constant_coeff.push_back(coeff);
        coeff_pos_2->second.obj_linear_coeff.push_back(var_idx1);
      } else {
        all_coeff new_coeff;
        new_coeff.obj_linear_constant_coeff.push_back(coeff);
        new_coeff.obj_linear_coeff.push_back(var_idx1);
        con.var_coeff->emplace(var_idx2, new_coeff);
      }
    }
  }
}

polynomial_constraint qp_solver::calc_cons(const polynomial_constraint &con, bool all_replace) {
  Float bound_offset = 0;
  int con_size = con.monomials->size();
  std::map<std::pair<int, int>, Float> mono_map;

  auto add_mono = [&](int var1, int var2, Float coeff) -> void {
    if (var1 > var2) std::swap(var1, var2);
    if (var1 == -1 && var2 == -1) {
      bound_offset += coeff;
      return;
    }

    auto key = std::make_pair(var1, var2);
    if (mono_map.find(key) == mono_map.end()) {
      if (std::abs(coeff) > preprocessor::eps) mono_map[key] = coeff;
    } else {
      mono_map[key] += coeff;
      if (std::abs(mono_map[key]) <= preprocessor::eps) mono_map.erase(key);
    }
  };

  for (const auto &mon : *con.monomials) {
    Float coeff = mon.coeff;
    if (mon.is_linear) {
      for (auto &[idx, c] : _vars_formula[mon.m_vars[0]]) {
        add_mono(idx, -1, c * coeff);
      }
    } else {
      int var1 = mon.m_vars[0], var2 = mon.is_multilinear ? mon.m_vars[1] : var1;
      for (auto &[idx1, c] : _vars_formula[var1]) {
        for (auto &[idx2, c2] : _vars_formula[var2]) {
          add_mono(idx1, idx2, c * c2 * coeff);
        }
      }
    }
  }
  if (mono_map.size() > con_size && !all_replace) return con;

  polynomial_constraint new_con;
  new_con.name = con.name;
  new_con.is_equal = con.is_equal;
  new_con.is_less = con.is_less;
  new_con.bound = con.bound - bound_offset;
  add_monomials(new_con, mono_map);

  return new_con;
}

void qp_solver::update_cons(qp_solver *solver, bool all_replace) {
#ifdef PREPROCESSOR_LOG
  auto print_con = [&](const polynomial_constraint &con) {
    int num = 0;
    for (auto mono : *con.monomials) {
      if (num != 0) {
        fprintf(stderr, " + ");
      }
      if (mono.is_linear) {
        fprintf(stderr, "%0.15Lf * %s", mono.coeff, _vars[mono.m_vars[0]].name.c_str());
      } else {
        if (mono.is_multilinear) {
          fprintf(stderr, "%0.15Lf * %s * %s", mono.coeff, _vars[mono.m_vars[0]].name.c_str(),
                  _vars[mono.m_vars[1]].name.c_str());
        } else {
          fprintf(stderr, "%0.15Lf * %s ^ 2", mono.coeff, _vars[mono.m_vars[0]].name.c_str());
        }
      }
      num++;
    }
    if (con.is_equal)
      fprintf(stderr, " = ");
    else if (con.is_less)
      fprintf(stderr, " <= ");
    else
      fprintf(stderr, " >= ");
    fprintf(stderr, "%0.15Lf\n", con.bound);
  };
#endif

  for (int i = 0; i < (int)_constraints.size(); ++i) {
    if (_removed_cons[i]) {
#ifdef PREPROCESSOR_LOG
      fprintf(stderr, "***[update_cons]*** constraint %s is removed\n", _constraints[i].name.c_str());
#endif
      continue;
    }

    const auto &con = _constraints[i];
    polynomial_constraint new_con = calc_cons(con, all_replace);
    add_constraint(solver, new_con);

#ifdef PREPROCESSOR_LOG
    fprintf(stderr, "***[update_cons]*** old_constraint %s: ", con.name.c_str());
    print_con(con);
    fprintf(stderr, "***[update_cons]*** new_constraint %s: ", new_con.name.c_str());
    print_con(new_con);
#endif
  }

  for (int i = 0; i < _vars_formula.size(); ++i) {
    if (_vars_formula[i].empty()) continue;
    if (_vars_formula[i].size() == 1 && _vars_formula[i][0].first == i) continue;

    polynomial_constraint new_con;
    new_con.name = _vars[i].name + "_equation";
    new_con.is_equal = true;
    new_con.is_less = false;

    std::map<std::pair<int, int>, Float> mono_map;
    mono_map[{-1, i}] = 1;
    for (auto [idx, coeff] : _vars_formula[i])
      if (idx == -1) {
        new_con.bound = coeff;
      } else {
        mono_map[{-1, idx}] = -coeff;
      }

    add_monomials(new_con, mono_map);
    add_constraint(solver, new_con);

#ifdef PREPROCESSOR_LOG
    fprintf(stderr, "***[update_cons]*** new_constraint %s: ", new_con.name.c_str());
    print_con(new_con);
#endif
  }
}

void qp_solver::copy_init(qp_solver *solver) {
  // copy options
  solver->_cut_off = _cut_off;
  solver->portfolio_mode = portfolio_mode;
  solver->output_mode = output_mode;

  // copy vars
  solver->_vars = _vars;
  solver->_bool_vars = _bool_vars;
  solver->_int_vars = _int_vars;
  solver->_vars_map = _vars_map;
  for (int i = 0; i < (int)_vars.size(); ++i) solver->_vars[i].constraints = std::make_shared<unordered_set<int>>();

  // copy obj
  solver->is_minimize = is_minimize;
  solver->_obj_constant = _obj_constant;
  solver->_object_monoials = _object_monoials;
  solver->_vars_in_obj = _vars_in_obj;

  // [TODO] check
}

qp_solver *qp_solver::preprocessing(int mat_limit, bool all_replace, bool init_flag, bool ls_flag) {
#ifdef PREPRESSOR_TIME
  auto start = std::chrono::high_resolution_clock::now();
#endif

  qp_solver *new_solver = new qp_solver;
  _removed_cons.resize(_constraints.size(), false);
  _vars_formula.resize(_vars.size());
  for (int i = 0; i < _vars.size(); ++i) _vars_formula[i] = {{i, 1}};

  copy_init(new_solver);
  union_vars();
  for (int i = 0; i < _union_size; ++i) {
    if (_union_vars[i].size() > mat_limit) continue;
    calc_freevars(i);
  }

  update_cons(new_solver, all_replace);
  new_solver->_union_size = _union_size;
  new_solver->_union_label = std::move(_union_label);
  new_solver->_union_cons = std::move(_union_cons);
  new_solver->_union_vars = std::move(_union_vars);
  new_solver->_vars_formula = std::move(_vars_formula);
  new_solver->_removed_cons = std::move(_removed_cons);
  new_solver->init_assignment = init_flag;
  new_solver->ls_assignment = ls_flag;
  _union_size = 0;

#ifdef PREPRESSOR_TIME
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;
  fprintf(stderr, "preprocessor time: %f\n", diff.count());
#endif
  return new_solver;
}

void qp_solver::adjust_assignments() {
  // if (has_assignment) assert(_vars_formula.empty());

  for (int i = 0; i < _vars_formula.size(); ++i) {
    if (_vars_formula[i].size() == 1 && _vars_formula[i][0].first == i) continue;

    Float value = 0;
    for (auto [idx, coeff] : _vars_formula[i]) {
      if (idx == -1) {
        value += coeff;
      } else {
        value += coeff * _cur_assignment[idx];
      }
    }

    // assert(!_vars[i].is_int && !_vars[i].is_bin);
    if (_vars[i].is_int || _vars[i].is_bin) {
      value = std::round(value);
      if (_vars[i].is_bin) {
        if (std::abs(value - 0) > preprocessor::eps && std::abs(value - 1) > preprocessor::eps) {
          continue;
        }
      }
    }
    if (_vars[i].has_lower && value - _vars[i].lower < -preprocessor::eps) {
      continue;
    } else if (_vars[i].has_upper && value - _vars[i].upper > preprocessor::eps) {
      continue;
    } else if (std::abs(_cur_assignment[i] - value) > preprocessor::eps) {
      execute_critical_move_mix(i, -_cur_assignment[i] + value);
    }
  }
}

void qp_solver::gauss_adjust_init(qp_solver *init_solver) {
  adjust_solver = std::make_shared<qp_solver>();
  adjust_solver->is_minimize = init_solver->is_minimize;
  adjust_solver->_vars = init_solver->_vars;
  adjust_solver->_constraints = init_solver->_constraints;
  adjust_solver->_bool_vars = init_solver->_bool_vars;
  adjust_solver->_int_vars = init_solver->_int_vars;
  adjust_solver->_var_num = init_solver->_var_num;
  adjust_solver->_bool_var_num = init_solver->_bool_var_num;
  adjust_solver->_int_var_num = init_solver->_int_var_num;
  adjust_solver->_cons_num = init_solver->_cons_num;
  adjust_solver->_object_monoials = init_solver->_object_monoials;
  adjust_solver->is_obj_quadratic = init_solver->is_obj_quadratic;
  adjust_solver->is_cons_quadratic = init_solver->is_cons_quadratic;
  adjust_solver->_obj_constant = init_solver->_obj_constant;
  adjust_solver->_vars_in_obj = init_solver->_vars_in_obj;
  adjust_solver->_vars_map = init_solver->_vars_map;
  adjust_solver->avg_bound = init_solver->avg_bound;
  adjust_solver->open_terminate = true;

  adjust_solver->portfolio_mode = 0;
  adjust_solver->parallel_solver = init_solver->parallel_solver;
  adjust_solver->parallel_tid = init_solver->parallel_tid;
  adjust_solver->output_mode = -1;
  adjust_solver->_cut_off = 0;
}

void qp_solver::preprocess() {
  qp_solver *new_solver = nullptr;
  if (portfolio_mode == 10)
    new_solver = preprocessing(2000, true, true, true);
  else if (portfolio_mode == 9)
    new_solver = preprocessing_new(2000);

  new_solver->_cut_off = _cut_off - 3 - TimeElapsed();
  new_solver->portfolio_mode = -9;
  new_solver->output_mode = -1;
  new_solver->attenuation_flag = true;
  new_solver->gauss_adjust_flag = true;
  new_solver->gauss_adjust_init(this);
  new_solver->local_search();
  // new_solver->print_best_solution();
  // exit(0);
  // printf(" | ");

  // printf("%0.15lf\n", new_solver->adjust_solver->_best_object_value);
  delete new_solver;
}

void qp_solver::attenuation(int var_idx, Float &change_value) {
  double ratio = TimeElapsed() / _cut_off, r = 1 - 0.5 / (std::exp(1) - 1) * (std::exp(ratio) - 1);

  int op = change_value > 0 ? 1 : -1;
  change_value *= r;
  if (_vars[var_idx].is_int) {
    change_value = std::round(change_value);
    if (change_value == 0) change_value = op;
  }
}

Float qp_solver::var_value_delta_in_obj(int var_idx, Float shift_value) {
  Float old_value = _cur_assignment[var_idx], new_value = old_value + shift_value;
  _propagationer->init_ranges();
  std::vector<std::pair<int, Float>> changes;
  changes.emplace_back(var_idx, new_value);
  _propagationer->propagate_linear(changes, true, false);

  std::unordered_map<int, Float> new_assignments = std::move(_propagationer->get_assignment());

  auto get_new_value = [&](int var_pos) {
    if (new_assignments.find(var_pos) != new_assignments.end())
      return new_assignments[var_pos];
    else
      return _cur_assignment[var_pos];
  };

  // fprintf(stderr, "***[var_value_delta_in_obj]*** now at var %s, shift %0.15lf\n", _vars[var_idx].name.c_str(),
  //         shift_value);
  // fprintf(stderr, "***[var_value_delta_in_obj]*** new_assignments: ");
  // for (auto [var_pos, value] : new_assignments) {
  //   fprintf(stderr, "%s: %0.15lf -> %0.15lf, ", _vars[var_pos].name.c_str(), _cur_assignment[var_pos], value);
  // }
  // fprintf(stderr, "\n");

  // fprintf(stderr, "***[var_value_delta_in_obj]*** obj: ");
  // for (auto mono : _object_monoials) {
  //   if (mono.is_linear) {
  //     fprintf(stderr, " |  %0.15lf * %s", mono.coeff, _vars[mono.m_vars[0]].name.c_str());
  //   } else {
  //     if (mono.is_multilinear) {
  //       fprintf(stderr, " |  %0.15lf * %s * %s", mono.coeff, _vars[mono.m_vars[0]].name.c_str(),
  //               _vars[mono.m_vars[1]].name.c_str());
  //     } else {
  //       fprintf(stderr, " |  %0.15lf * %s ^ 2", mono.coeff, _vars[mono.m_vars[0]].name.c_str());
  //     }
  //   }
  // }
  // fprintf(stderr, "\n");

  Float old_obj = 0, new_obj = 0;
  for (const auto &mon : _object_monoials) {
    if (mon.is_linear) {
      old_obj += mon.coeff * _cur_assignment[mon.m_vars[0]];
      new_obj += mon.coeff * get_new_value(mon.m_vars[0]);
    } else if (mon.is_multilinear) {
      old_obj += mon.coeff * _cur_assignment[mon.m_vars[0]] * _cur_assignment[mon.m_vars[1]];
      new_obj += mon.coeff * get_new_value(mon.m_vars[0]) * get_new_value(mon.m_vars[1]);
    } else {
      old_obj += mon.coeff * _cur_assignment[mon.m_vars[0]] * _cur_assignment[mon.m_vars[0]];
      new_obj += mon.coeff * get_new_value(mon.m_vars[0]) * get_new_value(mon.m_vars[0]);
    }
  }
  // fprintf(stderr, "***[var_value_delta_in_obj]*** old_obj: %0.15lf, new_obj: %0.15lf\n\n\n", old_obj, new_obj);
  return old_obj - new_obj;
}

}  // namespace solver