#include "propagation.h"
#include "sol.h"

namespace solver {

void qp_solver::update_cons_new(qp_solver *solver) {
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

  for (int i = 0; i < _vars_formula.size(); ++i) {
    if (_vars_formula[i].empty()) continue;
    if (_vars_formula[i].size() == 1 && _vars_formula[i][0].first == i) continue;

    auto formula_to_map = [&](const std::vector<std::pair<int, Float>> &formula, Float &bound, bool add_self) {
      std::map<std::pair<int, int>, Float> mono_map;
      if (add_self) mono_map[{-1, i}] = 1;
      for (auto [idx, coeff] : formula) {
        if (idx == -1) {
          bound = coeff;
        } else {
          mono_map[{-1, idx}] = -coeff;
        }
      }
      return mono_map;
    };
    if (_vars[i].is_int || _vars[i].is_bin) {
      polynomial_constraint new_con;
      new_con.name = _vars[i].name + "_equation";
      new_con.is_equal = true;
      new_con.is_less = false;

      std::map<std::pair<int, int>, Float> mono_map = std::move(formula_to_map(_vars_formula[i], new_con.bound, true));
      add_monomials(new_con, mono_map);
      add_constraint(solver, new_con);

      _vars_formula[i] = {{i, 1}};

#ifdef PREPROCESSOR_LOG
      fprintf(stderr, "***[update_cons_new]*** new_constraint %s: ", new_con.name.c_str());
      print_con(new_con);
#endif
    } else {
      if (_vars[i].has_lower) {
        polynomial_constraint new_con;
        new_con.name = _vars[i].name + "_lower";
        new_con.is_equal = false;
        new_con.is_less = false;

        std::map<std::pair<int, int>, Float> mono_map =
            std::move(formula_to_map(_vars_formula[i], new_con.bound, false));
        new_con.bound = _vars[i].lower - new_con.bound;
        add_monomials(new_con, mono_map);
        add_constraint(solver, new_con);

#ifdef PREPROCESSOR_LOG
        fprintf(stderr, "***[update_cons_new]*** new_constraint %s: ", new_con.name.c_str());
        print_con(new_con);
#endif
      }

      if (_vars[i].has_upper) {
        polynomial_constraint new_con;
        new_con.name = _vars[i].name + "_upper";
        new_con.is_equal = false;
        new_con.is_less = true;

        std::map<std::pair<int, int>, Float> mono_map =
            std::move(formula_to_map(_vars_formula[i], new_con.bound, false));
        new_con.bound = _vars[i].upper - new_con.bound;
        add_monomials(new_con, mono_map);
        add_constraint(solver, new_con);

#ifdef PREPROCESSOR_LOG
        fprintf(stderr, "***[update_cons_new]*** new_constraint %s: ", new_con.name.c_str());
        print_con(new_con);
#endif
      }
    }
  }

  for (int i = 0; i < (int)_constraints.size(); ++i) {
    if (_removed_cons[i]) {
#ifdef PREPROCESSOR_LOG
      fprintf(stderr, "***[update_cons_new]*** constraint %s is removed\n", _constraints[i].name.c_str());
#endif
      continue;
    }

    const auto &con = _constraints[i];
    polynomial_constraint new_con = calc_cons(con, true);
    add_constraint(solver, new_con);

#ifdef PREPROCESSOR_LOG
    fprintf(stderr, "***[update_cons_new]*** old_constraint %s: ", con.name.c_str());
    print_con(con);
    fprintf(stderr, "***[update_cons_new]*** new_constraint %s: ", new_con.name.c_str());
    print_con(new_con);
#endif
  }
}

qp_solver *qp_solver::preprocessing_new(int mat_limit) {
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

  update_cons_new(new_solver);
  new_solver->_union_size = _union_size;
  new_solver->_union_label = std::move(_union_label);
  new_solver->_union_cons = std::move(_union_cons);
  new_solver->_union_vars = std::move(_union_vars);
  new_solver->_vars_formula = std::move(_vars_formula);
  new_solver->_removed_cons = std::move(_removed_cons);
  new_solver->init_assignment = true;
  new_solver->ls_assignment = true;
  _union_size = 0;

#ifdef PREPRESSOR_TIME
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;
  fprintf(stderr, "preprocessor time: %f\n", diff.count());
#endif
  return new_solver;
}

}  // namespace solver