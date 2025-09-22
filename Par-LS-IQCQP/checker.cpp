#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "parallel.h"
#include "propagation.h"
#include "sol.h"

namespace checker {

constexpr Float eps = 1e-6;

Float obj_value;
std::vector<Float> cur_assignment;
std::set<std::string> has_assigned;

void check_solution(char *solution_file, solver::qp_solver *ls_qp_solver) {
  std::ifstream fin(solution_file);
  cur_assignment.resize(ls_qp_solver->_vars.size());
  for (std::string name, value; fin >> name >> value;) {
    if (name == "obj_value" || name == "Objective_Value") {
      obj_value = std::stod(value);
    } else {
      int idx = ls_qp_solver->_vars_map[name];
      cur_assignment[idx] = std::stod(value);
      has_assigned.insert(name);
    }
  }
  fin.close();

  bool valid = true;
  for (int i = 0; i < ls_qp_solver->_vars.size(); i++) {
    if (has_assigned.count(ls_qp_solver->_vars[i].name)) continue;
    valid = false;

    printf("-->  variable %s not assigned\n", ls_qp_solver->_vars[i].name.c_str());
  }

  if (!valid) return;

  for (int i = 0; i < ls_qp_solver->_vars.size(); i++) {
    if (ls_qp_solver->_vars[i].is_bin) {
      if (cur_assignment[i] != 0 && cur_assignment[i] != 1) {
        valid = false;
        printf("-->  variable %s is not binary, got: %0.15lf\n", ls_qp_solver->_vars[i].name.c_str(),
               cur_assignment[i]);
      }
    }
    if (ls_qp_solver->_vars[i].is_int) {
      if (cur_assignment[i] != std::round(cur_assignment[i])) {
        valid = false;
        printf("-->  variable %s is not integer, got: %0.15lf\n", ls_qp_solver->_vars[i].name.c_str(),
               cur_assignment[i]);
      }
    }
    if (ls_qp_solver->_vars[i].has_lower) {
      if (cur_assignment[i] - ls_qp_solver->_vars[i].lower < -eps) {
        valid = false;
        printf("-->  variable %s is not in lower bound, expected: >= %0.15lf, got: %0.15lf\n",
               ls_qp_solver->_vars[i].name.c_str(), ls_qp_solver->_vars[i].lower, cur_assignment[i]);
      }
    }
    if (ls_qp_solver->_vars[i].has_upper) {
      if (cur_assignment[i] - ls_qp_solver->_vars[i].upper > eps) {
        valid = false;
        printf("-->  variable %s is not in upper bound, expected: <= %lf, got: %lf\n",
               ls_qp_solver->_vars[i].name.c_str(), ls_qp_solver->_vars[i].upper, cur_assignment[i]);
      }
    }
  }

  auto calc_mono = [&](const solver::monomial &mono) -> Float {
    Float value = 0;
    if (mono.is_linear) {
      value += mono.coeff * cur_assignment[mono.m_vars[0]];
    } else if (mono.is_multilinear) {
      value += mono.coeff * cur_assignment[mono.m_vars[0]] * cur_assignment[mono.m_vars[1]];
    } else {
      value += mono.coeff * cur_assignment[mono.m_vars[0]] * cur_assignment[mono.m_vars[0]];
    }
    return value;
  };

  for (int i = 0; i < ls_qp_solver->_constraints.size(); i++) {
    Float value = 0;
    for (int j = 0; j < ls_qp_solver->_constraints[i].monomials->size(); j++) {
      value += calc_mono(ls_qp_solver->_constraints[i].monomials->at(j));
    }

    if (ls_qp_solver->_constraints[i].is_less) {
      if (value - ls_qp_solver->_constraints[i].bound > eps) {
        valid = false;
        printf("-->  constraint %s is violated, expected: <= %0.15lf, got: %0.15lf\n",
               ls_qp_solver->_constraints[i].name.c_str(), ls_qp_solver->_constraints[i].bound, value);
      }
    } else if (ls_qp_solver->_constraints[i].is_equal) {
      if (std::abs(value - ls_qp_solver->_constraints[i].bound) > eps) {
        valid = false;
        printf("-->  constraint %s is violated, expected: == %0.15lf, got: %0.15lf\n",
               ls_qp_solver->_constraints[i].name.c_str(), ls_qp_solver->_constraints[i].bound, value);
      }
    } else {
      if (ls_qp_solver->_constraints[i].bound - value > eps) {
        valid = false;
        printf("-->  constraint %s is violated, expected: >= %0.15lf, got: %0.15lf\n",
               ls_qp_solver->_constraints[i].name.c_str(), ls_qp_solver->_constraints[i].bound, value);
      }
    }
  }

  Float tot = 0;
  for (int i = 0; i < ls_qp_solver->_object_monoials.size(); i++) {
    tot += calc_mono(ls_qp_solver->_object_monoials[i]);
  }
  if (std::abs(tot - obj_value) > eps) {
    valid = false;
    printf("-->  objective is violated, expected: %0.15lf, got: %0.15lf\n", obj_value, tot);
  }

  if (valid) {
    printf("-->  solution is valid\n");
  } else {
    printf("-->  solution is invalid\n");
  }
}

}  // namespace checker

int main(int argc, char *argv[]) {
  solver::qp_solver *ls_qp_solver = new solver::qp_solver;
  ls_qp_solver->parse_mps(argv[1]);
  checker::check_solution(argv[2], ls_qp_solver);

  return 0;
}