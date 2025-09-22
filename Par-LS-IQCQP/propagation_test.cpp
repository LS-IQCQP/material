#include "propagation.h"

#include <stdio.h>

#include <sstream>

#include "sol.h"

bool fp_eq(Float a, Float b) {
  if (a == b) return true;

  static constexpr Float eps = 1e-6;
  return std::abs(a - b) < eps;
}

solver::qp_solver *parser(std::string lines) {
  solver::qp_solver *solver = new solver::qp_solver;

  std::istringstream iss(lines);
  string line;
  int stage = 0;
  bool is_min;
  while (getline(iss, line)) {
    if (line == "" || line.find("\\") != string::npos) continue;
    if (line == "Minimize") {
      is_min = true;
      stage = 1;
    } else if (line == "Maximize") {
      is_min = false;
      stage = 1;
    } else if (line == "Subject To")
      stage = 2;
    else if (line == "Bounds") {
      solver->_cons_num = solver->_constraints.size();
      stage = 3;
    } else if (line == "General" || line == "Generals")
      stage = 4;
    else if (line == "Binary" || line == "Binaries")
      stage = 5;
    else if (line == "End")
      break;

    if (stage == 1)
      solver->read_obj(line);
    else if (stage == 2)
      solver->read_cons(line);
    else if (stage == 3)
      solver->read_bounds(line);
    else if (stage == 4)
      solver->read_int(line);
    else if (stage == 5)
      solver->read_bin(line);
  }

  return solver;
}

void print_lines(std::string lines) {
  std::istringstream iss(lines);
  string line;
  while (getline(iss, line)) {
    if (line == "" || line.find("\\") != string::npos) continue;
    printf("[ %s ]\n", line.c_str());
  }
}

void fuzzy_test() {
  solver::range::fuzzy_range fr;

  // fuzzy range int test
  printf("=== Test fuzzy range's int variable ===\n");
  fr = solver::range::fuzzy_range(1.1, 2.2, true);
  printf("[ fuzzy_range(1.1, 2.2, true) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 2, 2 ]", fr.lower(), fr.upper());
  assert(fp_eq(fr.lower(), 2));
  assert(fp_eq(fr.upper(), 2));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 1 ]", fr.is_int());
  assert(fr.is_int() == 1);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 2(CONSTANT) ]", (int)fr.get_status());
  assert(fr.get_status() == solver::range::range::status::CONSTANT);
  printf("\n");

  fr = solver::range::fuzzy_range(1.1, 1.9, true);
  printf("[ fuzzy_range(1.1, 1.9, true) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 2, 1 ]", fr.lower(), fr.upper());
  assert(fp_eq(fr.lower(), 2));
  assert(fp_eq(fr.upper(), 1));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 1 ]", fr.is_int());
  assert(fr.is_int() == 1);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 1(INVALID) ]", (int)fr.get_status());
  assert(fr.get_status() == solver::range::range::status::INVALID);
  printf("\n");

  fr = solver::range::fuzzy_range(0.1, 3.9, true);
  printf("[ fuzzy_range(0.1, 3.9, true) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 1, 3 ]", fr.lower(), fr.upper());
  assert(fp_eq(fr.lower(), 1));
  assert(fp_eq(fr.upper(), 3));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 1 ]", fr.is_int());
  assert(fr.is_int() == 1);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr.get_status());
  assert(fr.get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr = solver::range::fuzzy_range(3, 3, true);
  printf("[ fuzzy_range(3, 3, true) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 3, 3 ]", fr.lower(), fr.upper());
  assert(fp_eq(fr.lower(), 3));
  assert(fp_eq(fr.upper(), 3));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 1 ]", fr.is_int());
  assert(fr.is_int() == 1);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 2(CONSTANT) ]", (int)fr.get_status());
  assert(fr.get_status() == solver::range::range::status::CONSTANT);
  printf("\n");

  fr = solver::range::fuzzy_range(3, 7, true);
  printf("[ fuzzy_range(3, 7, true) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 3, 7 ]", fr.lower(), fr.upper());
  assert(fp_eq(fr.lower(), 3));
  assert(fp_eq(fr.upper(), 7));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 1 ]", fr.is_int());
  assert(fr.is_int() == 1);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr.get_status());
  assert(fr.get_status() == solver::range::range::status::VALID);
  printf("\n");

  // fuzzy range float test
  printf("=== Test fuzzy range's float variable ===\n");
  fr = solver::range::fuzzy_range(1.1, 2.2, false);
  printf("[ fuzzy_range(1.1, 2.2, false) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 1.1, 2.2 ]", fr.lower(), fr.upper());
  assert(fp_eq(fr.lower(), 1.1));
  assert(fp_eq(fr.upper(), 2.2));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr.is_int());
  assert(fr.is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr.get_status());
  assert(fr.get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr = solver::range::fuzzy_range(2.2, 1.1, false);
  printf("[ fuzzy_range(2.2, 1.1, false) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 2.2, 1.1 ]", fr.lower(), fr.upper());
  assert(fp_eq(fr.lower(), 2.2));
  assert(fp_eq(fr.upper(), 1.1));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr.is_int());
  assert(fr.is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 1(INVALID) ]", (int)fr.get_status());
  assert(fr.get_status() == solver::range::range::status::INVALID);
  printf("\n");

  fr = solver::range::fuzzy_range(1.1, 1.10000001, false);
  printf("[ fuzzy_range(1.1, 1.10000001, false) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 1.1, 1.10000001 ]", fr.lower(), fr.upper());
  assert(fp_eq(fr.lower(), 1.1));
  assert(fp_eq(fr.upper(), 1.10000001));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr.is_int());
  assert(fr.is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 2(CONSTANT) ]", (int)fr.get_status());
  assert(fr.get_status() == solver::range::range::status::CONSTANT);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // plus operator test
  printf("=== Test plus operator ===\n");
  solver::range::fuzzy_range fr1(1.1, 2.2, false), fr2(2.4, 3.6, false);
  auto fr3 = fr1 + fr2;
  printf("[ fuzzy_range(1.1, 2.2, false) + fuzzy_range(2.4, 3.6, false) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 3.5, 5.8 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), 3.5));
  assert(fp_eq(fr3->upper(), 5.8));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, false), fr2 = solver::range::fuzzy_range(2.4, 3.6, true);
  fr3 = fr1 + fr2;
  printf("[ fuzzy_range(1.1, 2.2, false) + fuzzy_range(2.4, 3.6, true) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 4.1, 5.2 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), 4.1));
  assert(fp_eq(fr3->upper(), 5.2));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, true), fr2 = solver::range::fuzzy_range(2.4, 3.6, false);
  fr3 = fr1 + fr2;
  printf("[ fuzzy_range(1.1, 2.2, true) + fuzzy_range(2.4, 3.6, false) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 4.4, 5.6 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), 4.4));
  assert(fp_eq(fr3->upper(), 5.6));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, true), fr2 = solver::range::fuzzy_range(2.4, 3.6, true);
  fr3 = fr1 + fr2;
  printf("[ fuzzy_range(1.1, 2.2, true) + fuzzy_range(2.4, 3.6, true) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 5, 5 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), 5));
  assert(fp_eq(fr3->upper(), 5));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 1 ]", fr3->is_int());
  assert(fr3->is_int() == 1);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 2(CONSTANT) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::CONSTANT);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // minus operator test
  printf("=== Test minus operator ===\n");
  fr1 = solver::range::fuzzy_range(1.1, 2.2, false), fr2 = solver::range::fuzzy_range(2.4, 3.6, false);
  fr3 = fr1 - fr2;
  printf("[ fuzzy_range(1.1, 2.2, false) - fuzzy_range(2.4, 3.6, false) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ -2.5, -0.2 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), -2.5));
  assert(fp_eq(fr3->upper(), -0.2));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, false), fr2 = solver::range::fuzzy_range(2.4, 3.6, true);
  fr3 = fr1 - fr2;
  printf("[ fuzzy_range(1.1, 2.2, false) - fuzzy_range(2.4, 3.6, true) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ -1.9, -0.8 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), -1.9));
  assert(fp_eq(fr3->upper(), -0.8));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, true), fr2 = solver::range::fuzzy_range(2.4, 3.6, false);
  fr3 = fr1 - fr2;
  printf("[ fuzzy_range(1.1, 2.2, true) - fuzzy_range(2.4, 3.6, false) ]\n");
  printf("%45s: %-35s, actual [%Le, %Le]\n", "Range", "expect [ -1.6, -0.4 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), -1.6));
  assert(fp_eq(fr3->upper(), -0.4));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, true), fr2 = solver::range::fuzzy_range(2.4, 3.6, true);
  fr3 = fr1 - fr2;
  printf("[ fuzzy_range(1.1, 2.2, true) - fuzzy_range(2.4, 3.6, true) ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ -1, -1 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), -1));
  assert(fp_eq(fr3->upper(), -1));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 1 ]", fr3->is_int());
  assert(fr3->is_int() == 1);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 2(CONSTANT) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::CONSTANT);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // multiply operator test
  printf("=== Test multiply operator ===\n");
  fr1 = solver::range::fuzzy_range(1.1, 2.2, false);
  fr3 = fr1 * 2;
  printf("[ fuzzy_range(1.1, 2.2, false) * 2 ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 2.2, 4.4 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), 2.2));
  assert(fp_eq(fr3->upper(), 4.4));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(3, 3, true);
  fr3 = fr1 * 2;
  printf("[ fuzzy_range(3, 3, true) * 2 ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 6, 6 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), 6));
  assert(fp_eq(fr3->upper(), 6));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 2(CONSTANT) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::CONSTANT);
  printf("\n");

  fr1 = solver::range::fuzzy_range(3, 3, true);
  fr3 = fr1 * 0.5;
  printf("[ fuzzy_range(3, 3, true) * 0.5 ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 1.5, 1.5 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), 1.5));
  assert(fp_eq(fr3->upper(), 1.5));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 2(CONSTANT) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::CONSTANT);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, false);
  fr3 = fr1 * 0.5;
  printf("[ fuzzy_range(1.1, 2.2, false) * 0.5 ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 0.55, 1.1 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), 0.55));
  assert(fp_eq(fr3->upper(), 1.1));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, false);
  fr3 = fr1 * 0;
  printf("[ fuzzy_range(1.1, 2.2, false) * 0 ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ 0, 0 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), 0));
  assert(fp_eq(fr3->upper(), 0));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 2(CONSTANT) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::CONSTANT);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, false);
  fr3 = fr1 * -1;
  printf("[ fuzzy_range(1.1, 2.2, false) * -1 ]\n");
  printf("%45s: %-35s, actual [ %Le, %Le ]\n", "Range", "expect [ -2.2, -1.1 ]", fr3->lower(), fr3->upper());
  assert(fp_eq(fr3->lower(), -2.2));
  assert(fp_eq(fr3->upper(), -1.1));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr3->is_int());
  assert(fr3->is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr3->get_status());
  assert(fr3->get_status() == solver::range::range::status::VALID);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // intersect test
  printf("=== Test intersect ===\n");
  fr1 = solver::range::fuzzy_range(1.1, 2.2, false), fr2 = solver::range::fuzzy_range(2.4, 3.6, false);
  solver::range::range::status status = fr1.intersect(fr2);
  printf("[ fuzzy_range(1.1, 2.2, false) intersect fuzzy_range(2.4, 3.6, false) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 1(INVALID) ]", (int)status);
  assert(status == solver::range::range::status::INVALID);
  printf("%45s: %-35s, actual [%Le, %Le]\n", "Range", "expect [ 2.4, 2.2 ]", fr1.lower(), fr1.upper());
  assert(fp_eq(fr1.lower(), 2.4));
  assert(fp_eq(fr1.upper(), 2.2));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr1.is_int());
  assert(fr1.is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 1(INVALID) ]", (int)fr1.get_status());
  assert(fr1.get_status() == solver::range::range::status::INVALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, false), fr2 = solver::range::fuzzy_range(1.0, 3.6, false);
  status = fr1.intersect(fr2);
  printf("[ fuzzy_range(1.1, 2.2, false) intersect fuzzy_range(1.0, 3.6, false) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 0(VALID) ]", (int)status);
  assert(status == solver::range::range::status::VALID);
  printf("%45s: %-35s, actual [%Le, %Le]\n", "Range", "expect [ 1.1, 2.2 ]", fr1.lower(), fr1.upper());
  assert(fp_eq(fr1.lower(), 1.1));
  assert(fp_eq(fr1.upper(), 2.2));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr1.is_int());
  assert(fr1.is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr1.get_status());
  assert(fr1.get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(1.1, 2.2, false), fr2 = solver::range::fuzzy_range(1.7, 3.6, false);
  status = fr1.intersect(fr2);
  printf("[ fuzzy_range(1.1, 2.2, false) intersect fuzzy_range(1.7, 3.6, false) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 0(VALID) ]", (int)status);
  assert(status == solver::range::range::status::VALID);
  printf("%45s: %-35s, actual [%Le, %Le]\n", "Range", "expect [ 1.7, 2.2 ]", fr1.lower(), fr1.upper());
  assert(fp_eq(fr1.lower(), 1.7));
  assert(fp_eq(fr1.upper(), 2.2));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 0 ]", fr1.is_int());
  assert(fr1.is_int() == 0);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 0(VALID) ]", (int)fr1.get_status());
  assert(fr1.get_status() == solver::range::range::status::VALID);
  printf("\n");

  fr1 = solver::range::fuzzy_range(2, 3, true), fr2 = solver::range::fuzzy_range(2.5, 3.6, false);
  status = fr1.intersect(fr2);
  printf("[ fuzzy_range(2, 3, true) intersect fuzzy_range(2.5, 3.6, false) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 2(CONSTANT) ]", (int)status);
  assert(status == solver::range::range::status::CONSTANT);
  printf("%45s: %-35s, actual [%Le, %Le]\n", "Range", "expect [ 3, 3 ]", fr1.lower(), fr1.upper());
  assert(fp_eq(fr1.lower(), 3));
  assert(fp_eq(fr1.upper(), 3));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 1 ]", fr1.is_int());
  assert(fr1.is_int() == 1);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 2(CONSTANT) ]", (int)fr1.get_status());
  assert(fr1.get_status() == solver::range::range::status::CONSTANT);
  printf("\n");

  fr1 = solver::range::fuzzy_range(2, 3, true), fr2 = solver::range::fuzzy_range(3.5, 4.6, true);
  status = fr1.intersect(fr2);
  printf("[ fuzzy_range(2, 3, true) intersect fuzzy_range(3.5, 4.6, true) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 1(INVALID) ]", (int)status);
  assert(status == solver::range::range::status::INVALID);
  printf("%45s: %-35s, actual [%Le, %Le]\n", "Range", "expect [ 4, 3 ]", fr1.lower(), fr1.upper());
  assert(fp_eq(fr1.lower(), 4));
  assert(fp_eq(fr1.upper(), 3));
  printf("%45s: %-35s, actual [ %d ]\n", "is_int", "expect [ 1 ]", fr1.is_int());
  assert(fr1.is_int() == 1);
  printf("%45s: %-35s, actual [ %d ]\n", "get_status", "expect [ 1(INVALID) ]", (int)fr1.get_status());
  assert(fr1.get_status() == solver::range::range::status::INVALID);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // propagate_linear test
  printf("=== Test propagate_linear ===\n");

  std::string lines =
      "Minimize\n"
      " obj: x1 + x2 + x3\n"
      "Subject To\n"
      " e1: x1 + x2 + x3 >= 1\n"
      " e2: x1 + x2 + x3 <= 5\n"
      "Bounds\n"
      " 0 <= x1 <= 5\n"
      " 0 <= x2 <= 5\n"
      " 0 <= x3 <= 5\n"
      "Binary\n"
      "End\n";
  solver::qp_solver *solver = parser(lines);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::FUZZY);
  int idx;

  print_lines(lines);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x1 range & is_int & is_fixed", "expect [ 0, 5 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x2 range & is_int & is_fixed", "expect [ 0, 5 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x3 range & is_int & is_fixed", "expect [ 0, 5 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);

  std::vector<std::pair<int, Float>> propagation_data = {{solver->_vars_map["x1"], 2}, {solver->_vars_map["x2"], 3}};
  bool res = solver->_propagationer->propagate_linear(propagation_data, false, false).size() > 0;
  printf("[ propagate_linear(x1=2, x2=3) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x1 range & is_int & is_fixed & changed",
         "expect [ 2, 2 | 0 | 1 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 2));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 2));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x2 range & is_int & is_fixed",
         "expect [ 3, 3 | 0 | 1 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 3));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 3));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x3 range & is_int & is_fixed",
         "expect [ 0, 0 | 0 | 1 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 0));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  printf("\n");

  std::string lines2 =
      "Minimize\n"
      " obj: x1 + x2 + x3\n"
      "Subject To\n"
      " e1: x1 + x2 + x3 >= 7\n"
      " e2: x1 + x2 + x3 <= 5\n"
      "Bounds\n"
      " 0 <= x1 <= 5\n"
      " 0 <= x2 <= 5\n"
      " 0 <= x3 <= 5\n"
      "Binary\n"
      "End\n";
  solver = parser(lines2);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::FUZZY);

  print_lines(lines2);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x1 range & is_int & is_fixed", "expect [ 0, 5 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x2 range & is_int & is_fixed", "expect [ 0, 5 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x3 range & is_int & is_fixed", "expect [ 0, 5 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);

  std::vector<std::pair<int, Float>> propagation_data2 = {{solver->_vars_map["x1"], 2}, {solver->_vars_map["x2"], 3}};
  res = solver->_propagationer->propagate_linear(propagation_data2, false, false).size() > 0;
  printf("[ propagate_linear(x1=2, x2=3) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 0 ]", res);
  assert(res == 0);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x1 range & is_int & is_fixed & changed",
         "expect [ 0, 5 | 0 | 0 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x2 range & is_int & is_fixed & changed",
         "expect [ 0, 5 | 0 | 0 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x3 range & is_int & is_fixed & changed",
         "expect [ 0, 5 | 0 | 0 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  printf("\n");

  std::string line3 =
      "Minimize\n"
      " obj: x1 + x2 + b1\n"
      "Subject To\n"
      " e1: x1 + x2 = 3.9\n"
      " e2: x1 + x2 + b1 >= 4.5\n"
      " e3: x1 + x2 + b1 <= 5\n"
      "Bounds\n"
      " x1 free\n"
      " x2 free\n"
      "Binary\n"
      " b1\n"
      "End\n";
  solver = parser(line3);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::FUZZY);

  print_lines(line3);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x1 range & is_int & is_fixed", "expect [ -INF, INF | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -std::numeric_limits<Float>::infinity()));
  assert(fp_eq(solver->_propagationer->get_upper(idx), std::numeric_limits<Float>::infinity()));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x2 range & is_int & is_fixed", "expect [ -INF, INF | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -std::numeric_limits<Float>::infinity()));
  assert(fp_eq(solver->_propagationer->get_upper(idx), std::numeric_limits<Float>::infinity()));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "b1 range & is_int & is_fixed", "expect [ 0, 1 | 1 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 1));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);

  std::vector<std::pair<int, Float>> propagation_data3 = {{solver->_vars_map["x1"], 1.5}};
  res = solver->_propagationer->propagate_linear(propagation_data3, false, false).size() > 0;
  printf("[ propagate_linear(x1=1.5) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x1 range & is_int & is_fixed & changed",
         "expect [ 1.5, 1.5 | 0 | 1 | 0 ]", solver->_propagationer->get_lower(idx),
         solver->_propagationer->get_upper(idx), solver->_propagationer->var_is_int(idx),
         solver->_propagationer->var_is_fixed(idx), solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 1.5));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 1.5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x2 range & is_int & is_fixed & changed",
         "expect [ 2.4, 2.4 | 0 | 1 | 0 ]", solver->_propagationer->get_lower(idx),
         solver->_propagationer->get_upper(idx), solver->_propagationer->var_is_int(idx),
         solver->_propagationer->var_is_fixed(idx), solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 2.4));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 2.4));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "b1 range & is_int & is_fixed & changed",
         "expect [ 1, 1 | 1 | 1 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 1));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 1));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  printf("\n");

  std::string line4 =
      "Minimize\n"
      " obj: x1 + x2 + x3 + b1\n"
      "Subject To\n"
      " e1: x1 + x2 = 4.1\n"
      " e2: x1 + b1 >= 4.5\n"
      " e3: x1 + x2 + b1 <= 5\n"
      "Bounds\n"
      " -infinity <= x1 <= 5\n"
      " x2 free\n"
      " 0 <= x3 <= 10\n"
      "Binary\n"
      " b1\n"
      "End\n";
  solver = parser(line4);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::FUZZY);

  print_lines(line4);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x1 range & is_int & is_fixed", "expect [ -INF, 5 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -std::numeric_limits<Float>::infinity()));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x2 range & is_int & is_fixed", "expect [ -INF, INF | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -std::numeric_limits<Float>::infinity()));
  assert(fp_eq(solver->_propagationer->get_upper(idx), std::numeric_limits<Float>::infinity()));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x3 range & is_int & is_fixed", "expect [ 0, 10 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 10));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "b1 range & is_int & is_fixed", "expect [ 0, 1 | 1 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 1));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);

  std::vector<std::pair<int, Float>> propagation_data4 = {{solver->_vars_map["x2"], -0.3}};
  res = solver->_propagationer->propagate_linear(propagation_data4, false, false).size() > 0;
  printf("[ propagate_linear(x2=-0.3) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 0 ]", res);
  assert(res == 0);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x1 range & is_int & is_fixed & changed",
         "expect [ -INF, 5 | 0 | 0 | 0 ]", solver->_propagationer->get_lower(idx),
         solver->_propagationer->get_upper(idx), solver->_propagationer->var_is_int(idx),
         solver->_propagationer->var_is_fixed(idx), solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -std::numeric_limits<Float>::infinity()));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x2 range & is_int & is_fixed & changed",
         "expect [ -INF, INF | 0 | 0 | 0 ]", solver->_propagationer->get_lower(idx),
         solver->_propagationer->get_upper(idx), solver->_propagationer->var_is_int(idx),
         solver->_propagationer->var_is_fixed(idx), solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -std::numeric_limits<Float>::infinity()));
  assert(fp_eq(solver->_propagationer->get_upper(idx), std::numeric_limits<Float>::infinity()));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x3 range & is_int & is_fixed & changed",
         "expect [ 0, 10 | 0 | 0 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 10));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "b1 range & is_int & is_fixed & changed",
         "expect [ 0, 1 | 1 | 0 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 1));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);

  std::vector<std::pair<int, Float>> propagation_data5 = {{solver->_vars_map["x2"], -0.4}};
  res = solver->_propagationer->propagate_linear(propagation_data5, false, false).size() > 0;
  printf("[ propagate_linear(x2=-0.4) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x1 range & is_int & is_fixed & changed",
         "expect [ 4.5, 4.5 | 0 | 1 | 0 ]", solver->_propagationer->get_lower(idx),
         solver->_propagationer->get_upper(idx), solver->_propagationer->var_is_int(idx),
         solver->_propagationer->var_is_fixed(idx), solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 4.5));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 4.5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x2 range & is_int & is_fixed & changed",
         "expect [ -0.4, -0.4 | 0 | 1 | 0 ]", solver->_propagationer->get_lower(idx),
         solver->_propagationer->get_upper(idx), solver->_propagationer->var_is_int(idx),
         solver->_propagationer->var_is_fixed(idx), solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -0.4));
  assert(fp_eq(solver->_propagationer->get_upper(idx), -0.4));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x3 range & is_int & is_fixed & changed",
         "expect [ 0, 10 | 0 | 0 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 10));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "b1 range & is_int & is_fixed & changed",
         "expect [ 0, 0 | 1 | 1 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 0));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  printf("\n");

  std::string line5 =
      "Minimize\n"
      " obj: x1 + x2 + x3 + b1\n"
      "Subject To\n"
      " e1: 5 x1 + x2 <= 4.1\n"
      " e2: - x1 + b1 = 0.375\n"
      "Bounds\n"
      " -4.5 <= x1 <= 5\n"
      " x2 free\n"
      " 0 <= x3 <= 10\n"
      "Binary\n"
      " b1\n"
      "End\n";
  solver = parser(line5);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::FUZZY);

  print_lines(line5);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x1 range & is_int & is_fixed", "expect [ -4.5, 5 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -4.5));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 5));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x2 range & is_int & is_fixed", "expect [ -INF, INF | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -std::numeric_limits<Float>::infinity()));
  assert(fp_eq(solver->_propagationer->get_upper(idx), std::numeric_limits<Float>::infinity()));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "x3 range & is_int & is_fixed", "expect [ 0, 10 | 0 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 10));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d ]\n", "b1 range & is_int & is_fixed", "expect [ 0, 1 | 1 | 0 ]",
         solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 1));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);

  std::vector<std::pair<int, Float>> propagation_data6 = {{solver->_vars_map["x2"], 3.475}};
  res = solver->_propagationer->propagate_linear(propagation_data6, false, false).size() > 0;
  printf("[ propagate_linear(x2=3.475) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "return result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x1 range & is_int & is_fixed & changed",
         "expect [ -4.5, 0.125 | 0 | 0 | 0 ]", solver->_propagationer->get_lower(idx),
         solver->_propagationer->get_upper(idx), solver->_propagationer->var_is_int(idx),
         solver->_propagationer->var_is_fixed(idx), solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), -4.5));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 0.125));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x2 range & is_int & is_fixed & changed",
         "expect [ 3.475, 3.475 | 0 | 1 | 0 ]", solver->_propagationer->get_lower(idx),
         solver->_propagationer->get_upper(idx), solver->_propagationer->var_is_int(idx),
         solver->_propagationer->var_is_fixed(idx), solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 3.475));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 3.475));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 1);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "x3 range & is_int & is_fixed & changed",
         "expect [ 0, 10 | 0 | 0 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 10));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%45s: %-35s, actual [ %Le, %Le | %d | %d | %d ]\n", "b1 range & is_int & is_fixed & changed",
         "expect [ 0, 1 | 1 | 0 | 0 ]", solver->_propagationer->get_lower(idx), solver->_propagationer->get_upper(idx),
         solver->_propagationer->var_is_int(idx), solver->_propagationer->var_is_fixed(idx),
         solver->_propagationer->var_is_changed(idx));
  assert(fp_eq(solver->_propagationer->get_lower(idx), 0));
  assert(fp_eq(solver->_propagationer->get_upper(idx), 1));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  assert(solver->_propagationer->var_is_fixed(idx) == 0);
  assert(solver->_propagationer->var_is_changed(idx) == 0);
  printf("\n");
}

void print_range(const std::list<std::pair<Float, Float>> &l, std::string end = "\n") {
  for (auto &p : l) {
    printf("(%Le, %Le) ", p.first, p.second);
  }
  printf("%s", end.c_str());
}

void assert_range(const std::list<std::pair<Float, Float>> &l, const std::list<std::pair<Float, Float>> &r) {
  auto it1 = l.begin();
  auto it2 = r.begin();
  while (it1 != l.end() && it2 != r.end()) {
    assert(fp_eq(it1->first, it2->first));
    assert(fp_eq(it1->second, it2->second));
    it1++;
    it2++;
  }
  assert(it1 == l.end());
  assert(it2 == r.end());
}

void quad_fuzzy_test() {
  solver::range::quad_fuzzy_range er1, er2;

  // quad_fuzzy_range's constructor test
  printf("=== Test quad_fuzzy range's constructor ===\n");
  er1 = solver::range::quad_fuzzy_range(1.1, 1.9, false);
  printf("[ quad_fuzzy_range(1.1, 1.9, false) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (1.1, 1.9) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{1.1, 1.9}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er1.is_int());
  assert(er1.is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range(1.1, 1.9, true);
  printf("[ quad_fuzzy_range(1.1, 1.9, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er1.is_int());
  assert(er1.is_int() == 1);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range();
  printf("[ quad_fuzzy_range() ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-INF, INF) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er1.is_int());
  assert(er1.is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1.1, 1.3}, {3.1, 3.3}}, false);
  printf("[ quad_fuzzy_range({{1.1, 1.3}, {3.1, 3.3}}, false) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (1.1, 1.3), (3.1, 3.3) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{1.1, 1.3}, {3.1, 3.3}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er1.is_int());
  assert(er1.is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1.1, 2.0000}, {3.01, 3.99}}, true);
  printf("[ quad_fuzzy_range({{1.1, 2.0000}, {3.01, 3.99}}, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (2, 2) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{2, 2}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er1.is_int());
  assert(er1.is_int() == 1);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // plus operator test
  printf("=== Test plus operator ===\n");
  er1 = solver::range::quad_fuzzy_range({{1.1, 1.3}, {3.1, 3.3}}, false);
  er2 = solver::range::quad_fuzzy_range({{2.1, 2.3}, {4.1, 4.3}}, false);
  auto er3 = er1 + er2;
  printf("[ quad_fuzzy_range({{1.1, 1.3}, {3.1, 3.3}}, false) + quad_fuzzy_range({{2.1, 2.3}, {4.1, 4.3}}, false) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (3.2, 3.6), (5.2, 5.6), (7.2, 7.6) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{3.2, 3.6}, {5.2, 5.6}, {7.2, 7.6}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{-std::numeric_limits<Float>::infinity(), 1}, {3, 4}}, false);
  er2 = solver::range::quad_fuzzy_range({{1, 3}}, true);
  er3 = er1 + er2;
  printf("[ quad_fuzzy_range({{-INF, 1}, {3, 4}}, false) + quad_fuzzy_range({{1, 3}}, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-INF, 7) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-std::numeric_limits<Float>::infinity(), 7}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, true);
  er2 = solver::range::quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, false);
  er3 = er1 + er2;
  printf("[ quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, true) + quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, false) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (2.1, 3.3), (4.1, 5.3) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{2.1, 3.3}, {4.1, 5.3}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, true);
  er2 = solver::range::quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, true);
  er3 = er1 + er2;
  printf("[ quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, true) + quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (3, 3), (5, 5) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{3, 3}, {5, 5}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er3->is_int());
  assert(er3->is_int() == 1);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // minus operator test
  printf("=== Test minus operator ===\n");
  er1 = solver::range::quad_fuzzy_range({{1.1, 1.3}, {3.1, 3.3}}, false);
  er2 = solver::range::quad_fuzzy_range({{2.1, 2.3}, {4.1, 4.3}}, false);
  er3 = er1 - er2;
  printf("[ quad_fuzzy_range({{1.1, 1.3}, {3.1, 3.3}}, false) - quad_fuzzy_range({{2.1, 2.3}, {4.1, 4.3}}, false) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-3.2, -2.8), (-1.2, -0.8), (0.8, 1.2) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-3.2, -2.8}, {-1.2, -0.8}, {0.8, 1.2}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{-std::numeric_limits<Float>::infinity(), 1}, {3, 4}}, false);
  er2 = solver::range::quad_fuzzy_range({{1, 3}}, true);
  er3 = er1 - er2;
  printf("[ quad_fuzzy_range({{-INF, 1}, {3, 4}}, false) - quad_fuzzy_range({{1, 3}}, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-INF, 3) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-std::numeric_limits<Float>::infinity(), 3}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, true);
  er2 = solver::range::quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, false);
  er3 = er1 - er2;
  printf("[ quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, true) - quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, false) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-3.3, -2.1), (-1.3, -0.1) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-3.3, -2.1}, {-1.3, -0.1}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, false);
  er2 = solver::range::quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, true);
  er3 = er1 - er2;
  printf("[ quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, false) - quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-3.9, -2.7), (-1.9, -0.7), (1.1, 1.3) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-3.9, -2.7}, {-1.9, -0.7}, {1.1, 1.3}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, true);
  er2 = solver::range::quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, true);
  er3 = er1 - er2;
  printf("[ quad_fuzzy_range({{0.1, 1.3}, {3.1, 3.3}}, true) - quad_fuzzy_range({{1.1, 2.3}, {3.1, 4.3}}, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-3, -3), (-1, -1) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-3, -3}, {-1, -1}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er3->is_int());
  assert(er3->is_int() == 1);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // multiply operator test
  printf("=== Test multiply operator ===\n");
  er1 = solver::range::quad_fuzzy_range({{-3, -1}, {1, 3}}, true);
  er2 = solver::range::quad_fuzzy_range({{-1, 1}}, true);
  er3 = er1 * er2;
  printf("[ quad_fuzzy_range({{-3, -1}, {1, 3}}, true) * quad_fuzzy_range({{-1, 1}}, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-3, 3) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-3, 3}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er3->is_int());
  assert(er3->is_int() == 1);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1, 3}}, true);
  er2 = solver::range::quad_fuzzy_range({{-3, -1}}, true);
  er3 = er1 * er2;
  printf("[ quad_fuzzy_range({{1, 3}}, true) * quad_fuzzy_range({{-3, 1}}, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-9, -1) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-9, -1}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er3->is_int());
  assert(er3->is_int() == 1);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1, 3}, {2, 4}}, false);
  er2 = solver::range::quad_fuzzy_range({{-1, 1}}, true);
  er3 = er1 * er2;
  printf("[ quad_fuzzy_range({{1, 3}, {2, 4}}, false) * quad_fuzzy_range({{-1, 1}}, true) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-4, 4) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-4, 4}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1, 3}, {2, 4}}, true);
  er2 = solver::range::quad_fuzzy_range({{-1, 1}}, false);
  er3 = er1 * er2;
  printf("[ quad_fuzzy_range({{1, 3}, {2, 4}}, true) * quad_fuzzy_range({{-1, 1}}, false) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-4, 4) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-4, 4}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1.1, 1.3}, {3.1, 3.3}}, false);
  er3 = er1.square();
  printf("[ quad_fuzzy_range({{1.1, 1.3}, {3.1, 3.3}}, false).square() ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (1.21, 1.69), (9.61, 10.89) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{1.21, 1.69}, {9.61, 10.89}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{-4, -2}}, true);
  er3 = er1.square();
  printf("[ quad_fuzzy_range({{-4, -2}}, true).square() ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (4, 16) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{4, 16}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er3->is_int());
  assert(er3->is_int() == 1);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{-10, -9}, {-1, 0}, {2, 3}, {4, 8}}, true);
  er3 = er1.square();
  printf("[ quad_fuzzy_range({{-10, -9}, {-1, 0}, {2, 3}, {4, 8}}, true).square() ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (0, 1), (4, 9), (16, 64), (81, 100) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{0, 1}, {4, 9}, {16, 64}, {81, 100}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er3->is_int());
  assert(er3->is_int() == 1);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1, 3}}, true);
  er3 = er1 * 2;
  printf("[ quad_fuzzy_range({{1, 3}}, true) * 2 ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (2, 6) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{2, 6}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1, 3}, {3, 5}}, true);
  er3 = er1 * -2;
  printf("[ quad_fuzzy_range({{1, 3}, {3, 5}}, true) * -2 ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-10, -6), (-6, -2) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-10, -6}, {-6, -2}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1, 3}, {3, 5}}, false);
  er3 = er1 * 0;
  printf("[ quad_fuzzy_range({{1, 3}, {3, 5}}, false) * 0 ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (0, 0) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{0, 0}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er3->is_int());
  assert(er3->is_int() == 0);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // intersect test
  printf("=== Test intersect ===\n");
  er1 = solver::range::quad_fuzzy_range({{-std::numeric_limits<Float>::infinity(), 1}, {3, 4}}, false);
  er2 = solver::range::quad_fuzzy_range({{1, 3}}, true);
  auto status = er1.intersect(er2);
  printf("[ quad_fuzzy_range({{-INF, 1}, {3, 4}}, false).intersect(quad_fuzzy_range({{1, 3}}, true)) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 0(VALID) ]", (int)status);
  assert(status == solver::range::range::status::VALID);
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (1, 1), (3, 3) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{1, 1}, {3, 3}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 0 ]", er1.is_int());
  assert(er1.is_int() == 0);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1, 3}}, true);
  er2 = solver::range::quad_fuzzy_range({{-std::numeric_limits<Float>::infinity(), 1}, {3, 4}}, false);
  status = er1.intersect(er2);
  printf("[ quad_fuzzy_range({{1, 3}}, true).intersect(quad_fuzzy_range({{-INF, 1}, {3, 4}}, false)) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 0(VALID) ]", (int)status);
  assert(status == solver::range::range::status::VALID);
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (1, 1), (3, 3) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{1, 1}, {3, 3}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er1.is_int());
  assert(er1.is_int() == 1);
  printf("\n");

  er1 = solver::range::quad_fuzzy_range({{1, 3}, {5, 5}}, true);
  er2 = solver::range::quad_fuzzy_range({{2.2, 4.1}}, false);
  status = er1.intersect(er2);
  printf("[ quad_fuzzy_range({{1, 3}, {5, 5}}, true).intersect(quad_fuzzy_range({{2.2, 4.1}}, false)) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 2(CONSTANT) ]", (int)status);
  assert(status == solver::range::range::status::CONSTANT);
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (3, 3) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{3, 3}});
  printf("%10s: %-55s, actual [ %d ]\n", "Is_int", "expect [ 1 ]", er1.is_int());
  assert(er1.is_int() == 1);
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // propagate_quadratic test
  printf("=== Test propagate_quadratic ===\n");
  std::string line =
      "Minimize\n"
      " obj: x1 + x2 + x3 + b1\n"
      "Subject To\n"
      " e1: x1 + x2 = 4.1\n"
      " e2: x1 + b1 >= 4.5\n"
      " e3: x1 + x2 + b1 <= 5\n"
      "Bounds\n"
      " -infinity <= x1 <= 5\n"
      " x2 free\n"
      " 0 <= x3 <= 10\n"
      "Binary\n"
      " b1\n"
      "End\n";
  auto solver = parser(line);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::QUAD_FUZZY);
  int idx;

  print_lines(line);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (-INF, 5) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{-std::numeric_limits<Float>::infinity(), 5}});
  printf("%10s: %-55s, actual [ %d ]\n", "x1 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (-INF, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx),
               {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  printf("%10s: %-55s, actual [ %d ]\n", "x2 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%10s: %-55s, actual [ ", "x3 range", "expect [ (0, 10) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 10}});
  printf("%10s: %-55s, actual [ %d ]\n", "x3 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 1}});
  printf("%10s: %-55s, actual [ %d ]\n", "b1 is_int", "expect [ 1 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  printf("\n");

  std::vector<std::pair<int, Float>> propagation_data1 = {{solver->_vars_map["x2"], -0.3}};
  bool res = solver->_propagationer->propagate_quadratic(propagation_data1, false, false).size() > 0;
  printf("[ propagate_quadratic(x2=-0.3) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 0 ]", res);
  assert(res == 0);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (-INF, 5) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{-std::numeric_limits<Float>::infinity(), 5}});
  printf("%10s: %-55s, actual [ %d ]\n", "x1 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (-INF, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx),
               {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  printf("%10s: %-55s, actual [ %d ]\n", "x2 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%10s: %-55s, actual [ ", "x3 range", "expect [ (0, 10) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 10}});
  printf("%10s: %-55s, actual [ %d ]\n", "x3 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 1}});
  printf("%10s: %-55s, actual [ %d ]\n", "b1 is_int", "expect [ 1 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  printf("\n");

  std::vector<std::pair<int, Float>> propagation_data2 = {{solver->_vars_map["x2"], -0.4}};
  res = solver->_propagationer->propagate_quadratic(propagation_data2, false, false).size() > 0;
  printf("[ propagate_quadratic(x2=-0.4) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (4.5, 4.5) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{4.5, 4.5}});
  printf("%10s: %-55s, actual [ %d ]\n", "x1 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (-0.4, -0.4) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{-0.4, -0.4}});
  printf("%10s: %-55s, actual [ %d ]\n", "x2 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["x3"];
  printf("%10s: %-55s, actual [ ", "x3 range", "expect [ (0, 10) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 10}});
  printf("%10s: %-55s, actual [ %d ]\n", "x3 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 0) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}});
  printf("%10s: %-55s, actual [ %d ]\n", "b1 is_int", "expect [ 1 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  printf("\n");

  line =
      "Minimize\n"
      " obj: x1 + b1\n"
      "Subject To\n"
      " e1: [ x2 * x1 - x1^2 ] + b1 = 3\n"
      "Bounds\n"
      " x1 free\n"
      " 3 <= x2 <= infinity\n"
      "Binary\n"
      " b1\n"
      "End\n";
  solver = parser(line);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::QUAD_FUZZY);

  print_lines(line);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (-INF, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx),
               {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  printf("%10s: %-55s, actual [ %d ]\n", "x1 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (3, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{3, std::numeric_limits<Float>::infinity()}});
  printf("%10s: %-55s, actual [ %d ]\n", "x2 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 1}});
  printf("%10s: %-55s, actual [ %d ]\n", "b1 is_int", "expect [ 1 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  printf("\n");

  std::vector<std::pair<int, Float>> propagation_data3 = {{solver->_vars_map["b1"], 1}};
  res = solver->_propagationer->propagate_quadratic(propagation_data3, false, false).size() > 0;
  printf("[ propagate_quadratic(b1=1) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (0, 1), (2, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 1}, {2, std::numeric_limits<Float>::infinity()}});
  printf("%10s: %-55s, actual [ %d ]\n", "x1 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (3, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{3, std::numeric_limits<Float>::infinity()}});
  printf("%10s: %-55s, actual [ %d ]\n", "x2 is_int", "expect [ 0 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 0);
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{1, 1}});
  printf("%10s: %-55s, actual [ %d ]\n", "b1 is_int", "expect [ 1 ]", solver->_propagationer->var_is_int(idx));
  assert(solver->_propagationer->var_is_int(idx) == 1);
  printf("\n");

  // [TODO] add more tests
}

void exact_test() {
  solver::range::exact_range er1, er2;

  // exact_range's constructor test
  printf("=== Test exact range's constructor ===\n");
  er1 = solver::range::exact_range(2, 3);
  printf("[ exact_range(2, 3) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (2, 3) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{2, 3}});
  printf("\n");

  er1 = solver::range::exact_range();
  printf("[ exact_range() ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-INF, INF) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  printf("\n");

  er1 = solver::range::exact_range({{1, 2}, {3, 4}});
  printf("[ exact_range({1, 2}, {3, 4}) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (1, 2), (3, 4) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{1, 2}, {3, 4}});
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // plus operator test
  printf("=== Test plus operator ===\n");
  er1 = solver::range::exact_range({{1.1, 1.3}, {3.1, 3.3}});
  er2 = solver::range::exact_range({{2.1, 2.3}, {4.1, 4.3}});
  auto er3 = er1 + er2;
  printf("[ exact_range({{1.1, 1.3}, {3.1, 3.3}}) + exact_range({{2.1, 2.3}, {4.1, 4.3}}) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (3.2, 3.6), (5.2, 5.6), (7.2, 7.6) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{3.2, 3.6}, {5.2, 5.6}, {7.2, 7.6}});
  printf("\n");

  er1 = solver::range::exact_range({{-std::numeric_limits<Float>::infinity(), 1}, {3, 4}});
  er2 = solver::range::exact_range({{1, 3}});
  er3 = er1 + er2;
  printf("[ exact_range({{-INF, 1}, {3, 4}}) + exact_range({{1, 3}}) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-INF, 7) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-std::numeric_limits<Float>::infinity(), 7}});
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // minus operator test
  printf("=== Test minus operator ===\n");
  er1 = solver::range::exact_range({{1.1, 1.3}, {3.1, 3.3}});
  er2 = solver::range::exact_range({{2.1, 2.3}, {4.1, 4.3}});
  er3 = er1 - er2;
  printf("[ exact_range({{1.1, 1.3}, {3.1, 3.3}}) - exact_range({{2.1, 2.3}, {4.1, 4.3}}) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-3.2, -2.8), (-1.2, -0.8), (0.8, 1.2) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-3.2, -2.8}, {-1.2, -0.8}, {0.8, 1.2}});
  printf("\n");

  er1 = solver::range::exact_range({{-std::numeric_limits<Float>::infinity(), 1}, {3, 4}});
  er2 = solver::range::exact_range({{1, 3}});
  er3 = er1 - er2;
  printf("[ exact_range({{-INF, 1}, {3, 4}}) - exact_range({{1, 3}}) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-INF, 3) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-std::numeric_limits<Float>::infinity(), 3}});
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // multiply operator test
  printf("=== Test multiply operator ===\n");
  er1 = solver::range::exact_range({{-3, -1}, {1, 3}});
  er2 = solver::range::exact_range({{-1, 1}});
  er3 = er1 * er2;
  printf("[ exact_range({{-3, -1}, {1, 3}}) * exact_range({{-1, 1}}) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-3, 3) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-3, 3}});
  printf("\n");

  er1 = solver::range::exact_range({{1, 3}});
  er2 = solver::range::exact_range({{-3, -1}});
  er3 = er1 * er2;
  printf("[ exact_range({{1, 3}}) * exact_range({{-3, -1}}) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-9, -1) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-9, -1}});
  printf("\n");

  er1 = solver::range::exact_range({{-1, 3}});
  er2 = solver::range::exact_range({{-1, 3}});
  er3 = er1 * er2;
  printf("[ exact_range({{-1, 3}}) * exact_range({{-1, 3}}) ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-3, 9) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-3, 9}});
  printf("\n");

  er1 = solver::range::exact_range({{-1, 3}});
  er3 = er1.square();
  printf("[ exact_range({{-1, 3}}).square() ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (0, 9) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{0, 9}});
  printf("\n");

  er1 = solver::range::exact_range({{-4, -2}});
  er3 = er1.square();
  printf("[ exact_range({{-4, -2}}).square() ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (4, 16) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{4, 16}});
  printf("\n");

  er1 = solver::range::exact_range({{1, 3}});
  er3 = er1.square();
  printf("[ exact_range({{1, 3}}).square() ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (1, 9) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{1, 9}});
  printf("\n");

  er1 = solver::range::exact_range({{-10, -9}, {-1, 0}, {2, 3}, {4, 8}});
  er3 = er1.square();
  printf("[ exact_range({{-10, -9}, {-1, 0} , {2, 3}, {4, 8}}).square() ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (0, 1), (4, 9), (16, 64), (81, 100) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{0, 1}, {4, 9}, {16, 64}, {81, 100}});
  printf("\n");

  er1 = solver::range::exact_range({{1, 3}});
  er3 = er1 * 2;
  printf("[ exact_range({{1, 3}}) * 2 ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (2, 6) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{2, 6}});
  printf("\n");

  er1 = solver::range::exact_range({{1, 3}, {3, 5}});
  er3 = er1 * -2;
  printf("[ exact_range({{1, 3}, {3, 5}}) * -2 ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (-10, -6), (-6, -2) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-10, -6}, {-6, -2}});
  printf("\n");

  er1 = solver::range::exact_range({{1, 3}, {3, 5}});
  er3 = er1 * 0;
  printf("[ exact_range({{1, 3}, {3, 5}}) * 0 ]\n");
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (0, 0) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{0, 0}});
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // intersect test
  printf("=== Test intersect ===\n");
  er1 = solver::range::exact_range({{-std::numeric_limits<Float>::infinity(), 1}, {3, 4}});
  er2 = solver::range::exact_range({{1, 3}});
  solver::range::range::status status = er1.intersect(er2);
  printf("[ exact_range({{-INF, 1}, {3, 4}}).intersect(exact_range({{1, 3}}) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "Result", "expect [ 0(VALID) ]", (int)status);
  assert(status == solver::range::range::status::VALID);
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (1, 1), (3, 3) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{1, 1}, {3, 3}});
  printf("\n");

  er1 = solver::range::exact_range({{1, 3}});
  er2 = solver::range::exact_range({{-std::numeric_limits<Float>::infinity(), 0.9}, {3.1, 4}});
  status = er1.intersect(er2);
  printf("[ exact_range({{1, 3}}).intersect(exact_range({{-INF, 0.9}, {3.1, 4}}) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "Result", "expect [ 1(INVALID) ]", (int)status);
  assert(status == solver::range::range::status::INVALID);
  printf("%10s: %-55s, actual [ ", "Range", "expect [ ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {});
  printf("\n");

  er1 = solver::range::exact_range({{1, 3}, {5, 7}});
  er2 = solver::range::exact_range({{2, 4}, {6, 8}});
  status = er1.intersect(er2);
  printf("[ exact_range({{1, 3}, {5, 7}}).intersect(exact_range({{2, 4}, {6, 8}}) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "Result", "expect [ 0(VALID) ]", (int)status);
  assert(status == solver::range::range::status::VALID);
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (2, 3), (6, 7) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{2, 3}, {6, 7}});
  printf("\n");

  er1 = solver::range::exact_range({{1, 1}, {3, 3}});
  er2 = solver::range::exact_range({{-std::numeric_limits<Float>::infinity(), 2}});
  status = er1.intersect(er2);
  printf("[ exact_range({{1, 1}, {3, 3}}).intersect(exact_range({{-INF, 2}}) ]\n");
  printf("%45s: %-35s, actual [ %d ]\n", "Result", "expect [ 2(CONSTANT) ]", (int)status);
  assert(status == solver::range::range::status::CONSTANT);
  printf("%10s: %-55s, actual [ ", "Range", "expect [ (1, 1) ]");
  print_range(er1.get_range(), "]\n");
  assert_range(er1.get_range(), {{1, 1}});
  printf("\n");

  /*----------------------------------------------------------------------------*/

  solver::qp_solver *solver = new solver::qp_solver();
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::EXACT);

  // solve_quadratic(Float, Float, Float) test
  printf("=== Test solve_quadratic(Float, Float, Float) ===\n");
  er3 = solver->_propagationer->solve_quadratic(1, 2, 1);
  printf("[ solve_quadratic(1 * x^2 + 2 * x + 1 >= 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-INF, INF) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  printf("\n");

  er3 = solver->_propagationer->solve_quadratic(1, 0, -1);
  printf("[ solve_quadratic(1 * x^2 - 1 >= 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-INF, -1), (1, INF) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(),
               {{-std::numeric_limits<Float>::infinity(), -1}, {1, std::numeric_limits<Float>::infinity()}});
  printf("\n");

  er3 = solver->_propagationer->solve_quadratic(-1, -2, -1);
  printf("[ solve_quadratic(-1 * x^2 - 2 * x - 1 >= 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-1, -1) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-1, -1}});
  printf("\n");

  er3 = solver->_propagationer->solve_quadratic(1, 0, 1);
  printf("[ solve_quadratic(1 * x^2 + 1 >= 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-INF, INF) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  printf("\n");

  er3 = solver->_propagationer->solve_quadratic(-1, 0, -1);
  printf("[ solve_quadratic(-1 * x^2 - 1 >= 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {});
  printf("\n");

  er3 = solver->_propagationer->solve_quadratic(2, -12, 16);
  printf("[ solve_quadratic(2 * x^2 - 12 * x + 16 >= 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-INF, 2), (4, INF) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(),
               {{-std::numeric_limits<Float>::infinity(), 2}, {4, std::numeric_limits<Float>::infinity()}});
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // solve_quadratic(Float, range, range) test
  printf("=== Test solve_quadratic(Float, range, range) ===\n");

  er1 = solver::range::exact_range({{-2, std::numeric_limits<Float>::infinity()}});
  er2 = solver::range::exact_range({{-1, -1}});
  er3 = solver->_propagationer->solve_quadratic(-1, er1, er2);
  printf("[ solve_quadratic(-1 * x^2 + (-2, INF) * x + (-1, -1) = 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-1, -1), (0, INF) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-1, -1}, {0, std::numeric_limits<Float>::infinity()}});
  printf("\n");

  er1 = solver::range::exact_range({{-std::numeric_limits<Float>::infinity(), -2}});
  er2 = solver::range::exact_range({{-1, -1}});
  er3 = solver->_propagationer->solve_quadratic(-1, er1, er2);
  printf("[ solve_quadratic(-1 * x^2 + (-INF, -2) * x + (-1, -1) = 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-INF, 0) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-std::numeric_limits<Float>::infinity(), 0}});
  printf("\n");

  er1 = solver::range::exact_range({{3, std::numeric_limits<Float>::infinity()}});
  er2 = solver::range::exact_range({{-2, -2}});
  er3 = solver->_propagationer->solve_quadratic(-1, er1, er2);
  printf("[ solve_quadratic(-1 * x^2 + (3, INF) * x + (-2, -2) = 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (0, 1), (2, INF) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{0, 1}, {2, std::numeric_limits<Float>::infinity()}});
  printf("\n");

  er1 = solver::range::exact_range({{-2, -2}});
  er2 = solver::range::exact_range({{-std::numeric_limits<Float>::infinity(), -1}});
  er3 = solver->_propagationer->solve_quadratic(-1, er1, er2);
  printf("[ solve_quadratic(-1 * x^2 + (-2, -2) * x + (-INF, -1) = 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-1, -1) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-1, -1}});
  printf("\n");

  er1 = solver::range::exact_range({{-2, -2}});
  er2 = solver::range::exact_range({{-1, std::numeric_limits<Float>::infinity()}});
  er3 = solver->_propagationer->solve_quadratic(-1, er1, er2);
  printf("[ solve_quadratic(-1 * x^2 + (-2, -2) * x + (-1, INF) = 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-INF, INF) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  printf("\n");

  er1 = solver::range::exact_range({{-2, -2}, {3, std::numeric_limits<Float>::infinity()}});
  er2 = solver::range::exact_range({{-std::numeric_limits<Float>::infinity(), -1}});
  er3 = solver->_propagationer->solve_quadratic(-1, er1, er2);
  printf("[ solve_quadratic(-1 * x^2 + {(-2, -2), (3, INF)} * x + (-INF, -1) = 0) ]\n");
  printf("%10s: %-55s, actual [ ", "Result", "expect [ (-1, -1), (0, INF) ]");
  print_range(er3->get_range(), "]\n");
  assert_range(er3->get_range(), {{-1, -1}, {0, std::numeric_limits<Float>::infinity()}});
  printf("\n");

  /*----------------------------------------------------------------------------*/

  // propagate_quadratic test
  printf("=== Test propagate_quadratic ===\n");

  std::string line =
      "Minimize\n"
      " obj: x1 + x2 + x3 + b1\n"
      "Subject To\n"
      " e1: x1 + x2 = 4.1\n"
      " e2: x1 + b1 >= 4.5\n"
      " e3: x1 + x2 + b1 <= 5\n"
      "Bounds\n"
      " -infinity <= x1 <= 5\n"
      " x2 free\n"
      " 0 <= x3 <= 10\n"
      "Binary\n"
      " b1\n"
      "End\n";
  solver = parser(line);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::EXACT);
  int idx;

  print_lines(line);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (-INF, 5) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{-std::numeric_limits<Float>::infinity(), 5}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (-INF, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx),
               {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  idx = solver->_vars_map["x3"];
  printf("%10s: %-55s, actual [ ", "x3 range", "expect [ (0, 10) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 10}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 0), (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}, {1, 1}});

  std::vector<std::pair<int, Float>> propagation_data1 = {{solver->_vars_map["x2"], -0.3}};
  bool res = solver->_propagationer->propagate_quadratic(propagation_data1, false, false).size() > 0;
  printf("[ propagate_quadratic(x2=-0.3) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 0 ]", res);
  assert(res == 0);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (-INF, 5) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{-std::numeric_limits<Float>::infinity(), 5}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (-INF, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx),
               {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  idx = solver->_vars_map["x3"];
  printf("%10s: %-55s, actual [ ", "x3 range", "expect [ (0, 10) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 10}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 0), (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}, {1, 1}});

  std::vector<std::pair<int, Float>> propagation_data2 = {{solver->_vars_map["x2"], -0.4}};
  res = solver->_propagationer->propagate_quadratic(propagation_data2, false, false).size() > 0;
  printf("[ propagate_quadratic(x2=-0.4) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (4.5, 4.5) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{4.5, 4.5}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (-0.4, -0.4) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{-0.4, -0.4}});
  idx = solver->_vars_map["x3"];
  printf("%10s: %-55s, actual [ ", "x3 range", "expect [ (0, 10) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 10}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 0) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}});
  printf("\n");

  line =
      "Minimize\n"
      " obj: x1 + b1\n"
      "Subject To\n"
      " e1: [ x1^2 ] + b1 <= 1\n"
      "Bounds\n"
      " x1 free\n"
      "Binary\n"
      " b1\n"
      "End\n";
  solver = parser(line);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::EXACT);

  print_lines(line);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (-INF, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx),
               {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 0), (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}, {1, 1}});

  std::vector<std::pair<int, Float>> propagation_data3 = {{solver->_vars_map["b1"], 1}};
  res = solver->_propagationer->propagate_quadratic(propagation_data3, false, false).size() > 0;
  printf("[ propagate_quadratic(b1=1) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (0, 0) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{1, 1}});
  printf("\n");

  line =
      "Minimize\n"
      " obj: x1 + b1\n"
      "Subject To\n"
      " e1: [ x2 * x1 - x1^2 ] + b1 = 3\n"
      "Bounds\n"
      " x1 free\n"
      " 3 <= x2 <= infinity\n"
      "Binary\n"
      " b1\n"
      "End\n";
  solver = parser(line);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::EXACT);

  print_lines(line);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (-INF, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx),
               {{-std::numeric_limits<Float>::infinity(), std::numeric_limits<Float>::infinity()}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (3, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{3, std::numeric_limits<Float>::infinity()}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 0), (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}, {1, 1}});

  std::vector<std::pair<int, Float>> propagation_data4 = {{solver->_vars_map["b1"], 1}};
  res = solver->_propagationer->propagate_quadratic(propagation_data4, false, false).size() > 0;
  printf("[ propagate_quadratic(b1=1) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (0, 1), (2, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 1}, {2, std::numeric_limits<Float>::infinity()}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (3, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{3, std::numeric_limits<Float>::infinity()}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{1, 1}});
  printf("\n");

  line =
      "Minimize\n"
      " obj: x1 + b1\n"
      "Subject To\n"
      " e1: [ x2 * x1 - x1^2 ] + b1 = 3\n"
      "Bounds\n"
      " 1 <= x1 <= 1.5\n"
      " 3 <= x2 <= infinity\n"
      "Binary\n"
      " b1\n"
      "End\n";
  solver = parser(line);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::EXACT);

  print_lines(line);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (1, 1.5) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{1, 1.5}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (3, INF) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{3, std::numeric_limits<Float>::infinity()}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 0), (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}, {1, 1}});

  std::vector<std::pair<int, Float>> propagation_data5 = {{solver->_vars_map["b1"], 1}};
  res = solver->_propagationer->propagate_quadratic(propagation_data5, false, false).size() > 0;
  printf("[ propagate_quadratic(b1=1) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{1, 1}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (3, 3) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{3, 3}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{1, 1}});
  printf("\n");

  line =
      "Minimize\n"
      " obj: x1 + b1\n"
      "Subject To\n"
      " e1: [ b1 * x1 ] - x2 = 0.1\n"
      "Bounds\n"
      " 0.25 <= x1 <= 0.4\n"
      " 0.1 <= x2 <= 0.15\n"
      "Binary\n"
      " b1\n"
      "End\n";
  solver = parser(line);
  solver->_propagationer =
      std::make_unique<solver::qp_solver::propagation>(*solver, solver::qp_solver::propagation::range_type::EXACT);

  print_lines(line);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (0.25, 0.4) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0.25, 0.4}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (0.1, 0.15) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0.1, 0.15}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 0), (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}, {1, 1}});

  std::vector<std::pair<int, Float>> propagation_data6 = {{solver->_vars_map["b1"], 0}};
  res = solver->_propagationer->propagate_quadratic(propagation_data6, false, false).size() > 0;
  printf("[ propagate_quadratic(b1=0) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 0 ]", res);
  assert(res == 0);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (0.25, 0.4) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0.25, 0.4}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (0.1, 0.15) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0.1, 0.15}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (0, 0), (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0, 0}, {1, 1}});

  std::vector<std::pair<int, Float>> propagation_data7 = {{solver->_vars_map["b1"], 1}};
  res = solver->_propagationer->propagate_quadratic(propagation_data7, false, false).size() > 0;
  printf("[ propagate_quadratic(b1=1) ]\n");
  printf("%10s: %-55s, actual [ %d ]\n", "Result", "expect [ 1 ]", res);
  assert(res == 1);
  idx = solver->_vars_map["x1"];
  printf("%10s: %-55s, actual [ ", "x1 range", "expect [ (0.25, 0.25) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0.25, 0.25}});
  idx = solver->_vars_map["x2"];
  printf("%10s: %-55s, actual [ ", "x2 range", "expect [ (0.15, 0.15) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{0.15, 0.15}});
  idx = solver->_vars_map["b1"];
  printf("%10s: %-55s, actual [ ", "b1 range", "expect [ (1, 1) ]");
  print_range(solver->_propagationer->get_range(idx), "]\n");
  assert_range(solver->_propagationer->get_range(idx), {{1, 1}});
  printf("\n");
}

int main() {
  printf("==================================================\n");
  fuzzy_test();
  printf("==================================================\n");
  quad_fuzzy_test();
  printf("==================================================\n");
  exact_test();

  return 0;
}