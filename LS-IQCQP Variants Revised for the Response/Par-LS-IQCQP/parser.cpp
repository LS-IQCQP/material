#include <cassert>
#include <fstream>
#include <map>
#include <string>

#include "sol.h"

namespace mps {

int qc_num = 0;
bool is_int = false;
std::string obj_name = "";
std::vector<std::string> tokens;
std::map<std::string, int> cons_map;
std::unordered_set<int> noinit_var, ulower_var;
std::map<std::pair<int, int>, Float> qmatrix, qcmatrix;

bool read_token(std::ifstream &fin, std::string &token) {
  token.clear();
  char c;
  while (fin.get(c)) {
    if (c == ' ' || c == '\t') {
      continue;
    } else {
      fin.unget();
      break;
    }
  }
  if (!fin) return false;

  fin.get(c);
  if (c == '\n') {
    token = c;
    return true;
  } else {
    fin.unget();
  }

  while (fin.get(c)) {
    if (c == ' ' || c == '\t' || c == '\n') {
      fin.unget();
      break;
    }
    token += c;
  }
  return !token.empty();
}

void read_files(char *filename) {
  std::ifstream fin(filename);
  if (!fin) {
    std::cerr << "Error: cannot open file " << filename << std::endl;
    exit(1);
  }

  std::string token;
  while (read_token(fin, token)) {
    tokens.push_back(token);
  }
  std::reverse(tokens.begin(), tokens.end());
  fin.close();
}

bool next_token(std::string &token) {
  if (tokens.empty()) return false;
  token = tokens.back();
  return true;
}

bool pop_token() {
  if (tokens.empty()) return false;
  tokens.pop_back();
  return true;
}

bool pop_token(std::string &token) {
  if (tokens.empty()) return false;
  token = tokens.back();
  tokens.pop_back();
  return true;
}

}  // namespace mps

namespace solver {

void qp_solver::parse_rows() {
  std::string type;
  mps::pop_token(type);

  if (type == "N") {
    assert(mps::pop_token(mps::obj_name));
  } else {
    std::string name;
    mps::pop_token(name);

    _cons_num++;
    polynomial_constraint con;
    con.index = _cons_num;
    con.name = name;
    if (type == "L") {
      con.is_less = true;
    } else if (type == "G") {
      con.is_less = false;
    } else if (type == "E") {
      con.is_equal = true;
    } else {
      std::cerr << "Error: unknown row type " << type << std::endl;
      exit(1);
    }
    _constraints.push_back(con);
    mps::cons_map[name] = _cons_num;
  }
}

void qp_solver::parse_columns() {
  std::string var_name, obj_name;
  mps::pop_token(var_name);

  // allocate linear coeff
  while (mps::next_token(obj_name)) {
    if (obj_name == "\n") break;

    mps::pop_token();
    if (obj_name == "'MARKER'") {
      mps::pop_token(obj_name);
      if (obj_name == "'INTORG'") {
        assert(mps::is_int == false);
        mps::is_int = true;
      } else if (obj_name == "'INTEND'") {
        assert(mps::is_int == true);
        mps::is_int = false;
      } else {
        std::cerr << "Error: unknown marker " << obj_name << std::endl;
        exit(1);
      }
      continue;
    } else {
      // allocate var
      bool first_add = _vars_map.count(var_name) == 0;
      int var_idx = register_var(var_name);
      if (first_add && mps::is_int) {
        _vars[var_idx].is_int = true;
        _vars[var_idx].is_bin = true;
        mps::noinit_var.insert(var_idx);
        _int_vars.insert(var_idx);
        _bool_vars.insert(var_idx);
      }

      std::string value;
      assert(mps::pop_token(value));
      Float coeff = std::stold(value);

      if (obj_name == mps::obj_name) {
        if (!is_minimize) coeff = -coeff;
        if (coeff != 0) {
          _vars[var_idx].obj_constant_coeff += coeff;
          _vars[var_idx].is_in_obj = true;
          _vars_in_obj.insert(var_idx);

          // add monomial
          _vars[var_idx].obj_monomials->push_back(_object_monoials.size());
          monomial mono(var_idx, coeff, true);
          _object_monoials.push_back(mono);
        }
      } else {
        int con_idx = mps::cons_map[obj_name];

        if (coeff != 0) {
          _vars[var_idx].constraints->insert(con_idx);
          monomial mono(var_idx, coeff, true);
          _constraints[con_idx].monomials->push_back(mono);

          // build var's coeff
          polynomial_constraint *cur_con = &(_constraints[con_idx]);
          auto coeff_pos = cur_con->var_coeff->find(var_idx);
          if (coeff_pos != cur_con->var_coeff->end())
            coeff_pos->second.obj_constant_coeff += coeff;
          else {
            all_coeff new_coeff;
            new_coeff.obj_constant_coeff += coeff;
            cur_con->var_coeff->emplace(var_idx, new_coeff);
          }
        }
      }
    }
  }
}

void qp_solver::parse_rhs() {
  std::string identifier, cons;
  mps::pop_token(identifier);

  while (mps::next_token(cons)) {
    if (cons == "\n") break;
    mps::pop_token();

    std::string value;
    assert(mps::pop_token(value));
    assert(value != "\n");

    Float coeff = std::stold(value);
    if (cons == mps::obj_name) {
      _obj_constant += -coeff;
    } else {
      int con_idx = mps::cons_map[cons];
      _constraints[con_idx].bound = coeff;
      avg_bound += std::abs(coeff);
    }
  }
}

void qp_solver::parse_bounds() {
  std::string type;
  mps::pop_token(type);

  std::string identifier, var_name, value;
  assert(mps::pop_token(identifier));
  assert(identifier != "\n");
  assert(mps::pop_token(var_name));
  assert(var_name != "\n");

  if (type == "LO") {
    assert(mps::pop_token(value));
    assert(value != "\n");

    Float val = std::stold(value);
    int var_idx = register_var(var_name);
    _vars[var_idx].has_lower = true;
    _vars[var_idx].lower = val;

    if (mps::noinit_var.count(var_idx) > 0) {
      mps::noinit_var.erase(var_idx);
      _vars[var_idx].is_bin = false;
      if (_bool_vars.count(var_idx) > 0) _bool_vars.erase(var_idx);
    }
    mps::ulower_var.insert(var_idx);
  } else if (type == "LI") {
    assert(mps::pop_token(value));
    assert(value != "\n");

    int64_t val = std::stoll(value);
    int var_idx = register_var(var_name);
    _vars[var_idx].has_lower = true;
    _vars[var_idx].lower = val;
    _vars[var_idx].is_int = true;
    _int_vars.insert(var_idx);

    if (mps::noinit_var.count(var_idx) > 0) {
      mps::noinit_var.erase(var_idx);
      _vars[var_idx].is_bin = false;
      if (_bool_vars.count(var_idx) > 0) _bool_vars.erase(var_idx);
    }
    mps::ulower_var.insert(var_idx);
  } else if (type == "UP") {
    assert(mps::pop_token(value));
    assert(value != "\n");

    Float val = std::stold(value);
    int var_idx = register_var(var_name);
    _vars[var_idx].has_upper = true;
    _vars[var_idx].upper = val;

    if (mps::noinit_var.count(var_idx) > 0) {
      mps::noinit_var.erase(var_idx);
      _vars[var_idx].is_bin = false;
      if (_bool_vars.count(var_idx) > 0) _bool_vars.erase(var_idx);
    }
    if (mps::ulower_var.count(var_idx) == 0 && val < 0) _vars[var_idx].has_lower = false;
  } else if (type == "UI") {
    assert(mps::pop_token(value));
    assert(value != "\n");

    int64_t val = std::stoll(value);
    int var_idx = register_var(var_name);
    _vars[var_idx].has_upper = true;
    _vars[var_idx].upper = val;
    _vars[var_idx].is_int = true;
    _int_vars.insert(var_idx);

    if (mps::noinit_var.count(var_idx) > 0) {
      mps::noinit_var.erase(var_idx);
      _vars[var_idx].is_bin = false;
      if (_bool_vars.count(var_idx) > 0) _bool_vars.erase(var_idx);
    }
    if (mps::ulower_var.count(var_idx) == 0 && val < 0) _vars[var_idx].has_lower = false;
  } else if (type == "FX") {
    assert(mps::pop_token(value));
    assert(value != "\n");

    Float val = std::stold(value);
    int var_idx = register_var(var_name);
    _vars[var_idx].has_lower = _vars[var_idx].has_upper = true;
    _vars[var_idx].lower = _vars[var_idx].upper = val;

    _vars[var_idx].is_constant = true;
    _vars[var_idx].constant = val;

    if (mps::noinit_var.count(var_idx) > 0) {
      mps::noinit_var.erase(var_idx);
      _vars[var_idx].is_bin = false;
      if (_bool_vars.count(var_idx) > 0) _bool_vars.erase(var_idx);
    }
  } else if (type == "FR") {
    assert(mps::next_token(value));
    assert(value == "\n");

    int var_idx = register_var(var_name);
    _vars[var_idx].has_lower = _vars[var_idx].has_upper = false;

    if (mps::noinit_var.count(var_idx) > 0) {
      mps::noinit_var.erase(var_idx);
      _vars[var_idx].is_bin = false;
      if (_bool_vars.count(var_idx) > 0) _bool_vars.erase(var_idx);
    }
  } else if (type == "MI") {
    assert(mps::pop_token(value));
    assert(value == "\n");

    int var_idx = register_var(var_name);
    _vars[var_idx].has_lower = false;

    if (mps::noinit_var.count(var_idx) > 0) {
      mps::noinit_var.erase(var_idx);
      _vars[var_idx].is_bin = false;
      if (_bool_vars.count(var_idx) > 0) _bool_vars.erase(var_idx);
    }
    mps::ulower_var.insert(var_idx);
  } else if (type == "PL") {
    assert(mps::pop_token(value));
    assert(value == "\n");

    int var_idx = register_var(var_name);
    _vars[var_idx].has_upper = false;

    if (mps::noinit_var.count(var_idx) > 0) {
      mps::noinit_var.erase(var_idx);
      _vars[var_idx].is_bin = false;
      if (_bool_vars.count(var_idx) > 0) _bool_vars.erase(var_idx);
    }
  } else if (type == "BV") {
    assert(mps::pop_token(value));
    assert(value == "\n");

    int var_idx = register_var(var_name);
    _vars[var_idx].is_bin = true;
    _bool_vars.insert(var_idx);

    if (mps::noinit_var.count(var_idx) > 0) mps::noinit_var.erase(var_idx);
  } else {
    std::cerr << "Error: unknown bound type " << type << std::endl;
    exit(1);
  }
}

void qp_solver::parse_qcmatrix() {
  std::string var1, var2, val;
  assert(mps::pop_token(var1));
  assert(mps::pop_token(var2));
  assert(var1 != "\n" && var2 != "\n");
  assert(mps::pop_token(val));
  assert(val != "\n");

  int var_idx1 = register_var(var1), var_idx2 = register_var(var2);
  Float coeff = std::stold(val);

  if (var_idx1 > var_idx2) std::swap(var_idx1, var_idx2);
  if (var_idx1 == var_idx2) {
    _vars[var_idx1].constraints->insert(mps::qc_num);
    monomial mono(var_idx1, coeff, false);
    _constraints[mps::qc_num].monomials->push_back(mono);

    // build var's coeff
    polynomial_constraint *cur_con = &(_constraints[mps::qc_num]);
    auto coeff_pos = cur_con->var_coeff->find(var_idx1);
    if (coeff_pos != cur_con->var_coeff->end())
      coeff_pos->second.obj_quadratic_coeff += coeff;
    else {
      all_coeff new_coeff;
      new_coeff.obj_quadratic_coeff += coeff;
      cur_con->var_coeff->emplace(var_idx1, new_coeff);
    }
  } else {
    auto key = std::make_pair(var_idx1, var_idx2);
    auto pos = mps::qcmatrix.find(key);

    if (pos != mps::qcmatrix.end()) {
      coeff += pos->second;

      _vars[var_idx1].constraints->insert(mps::qc_num);
      _vars[var_idx2].constraints->insert(mps::qc_num);
      monomial mono(var_idx1, var_idx2, coeff, false);
      _constraints[mps::qc_num].monomials->push_back(mono);

      // build var's coeff
      polynomial_constraint *cur_con = &(_constraints[mps::qc_num]);
      auto coeff_pos_1 = cur_con->var_coeff->find(var_idx1);
      auto coeff_pos_2 = cur_con->var_coeff->find(var_idx2);
      if (coeff_pos_1 != cur_con->var_coeff->end()) {
        coeff_pos_1->second.obj_linear_constant_coeff.push_back(coeff);
        coeff_pos_1->second.obj_linear_coeff.push_back(var_idx2);
      } else {
        all_coeff new_coeff;
        new_coeff.obj_linear_constant_coeff.push_back(coeff);
        new_coeff.obj_linear_coeff.push_back(var_idx2);
        cur_con->var_coeff->emplace(var_idx1, new_coeff);
      }
      if (coeff_pos_2 != cur_con->var_coeff->end()) {
        coeff_pos_2->second.obj_linear_constant_coeff.push_back(coeff);
        coeff_pos_2->second.obj_linear_coeff.push_back(var_idx1);
      } else {
        all_coeff new_coeff;
        new_coeff.obj_linear_constant_coeff.push_back(coeff);
        new_coeff.obj_linear_coeff.push_back(var_idx1);
        cur_con->var_coeff->emplace(var_idx2, new_coeff);
      }
    } else {
      mps::qcmatrix[key] = coeff;
    }
  }
}

void qp_solver::parse_qmatrix() {
  std::string var1, var2, val;
  assert(mps::pop_token(var1));
  assert(mps::pop_token(var2));
  assert(var1 != "\n" && var2 != "\n");
  assert(mps::pop_token(val));
  assert(val != "\n");

  int var_idx1 = register_var(var1), var_idx2 = register_var(var2);
  Float coeff = std::stold(val);

  if (var_idx1 > var_idx2) std::swap(var_idx1, var_idx2);

  if (var_idx1 == var_idx2) {
    coeff /= 2;
    if (!is_minimize) coeff = -coeff;

    _vars[var_idx1].obj_quadratic_coeff += coeff;
    _vars[var_idx1].is_in_obj = true;
    _vars_in_obj.insert(var_idx1);
    _vars[var_idx1].obj_monomials->push_back(_object_monoials.size());
    monomial mono(var_idx1, coeff, false);
    _object_monoials.push_back(mono);
  } else {
    std::pair<int, int> key(var_idx1, var_idx2);
    auto pos = mps::qmatrix.find(key);

    if (pos != mps::qmatrix.end()) {
      coeff = (coeff + pos->second) / 2;
      if (!is_minimize) coeff = -coeff;

      _vars[var_idx1].is_in_obj = true;
      _vars[var_idx2].is_in_obj = true;
      _vars[var_idx1].obj_linear_coeff->push_back(var_idx2);
      _vars[var_idx1].obj_linear_constant_coeff->push_back(coeff);
      _vars[var_idx2].obj_linear_coeff->push_back(var_idx1);
      _vars[var_idx2].obj_linear_constant_coeff->push_back(coeff);
      _vars_in_obj.insert(var_idx1);
      _vars_in_obj.insert(var_idx2);

      _vars[var_idx1].obj_monomials->push_back(_object_monoials.size());
      _vars[var_idx2].obj_monomials->push_back(_object_monoials.size());
      monomial mono(var_idx1, var_idx2, coeff, false);
      _object_monoials.push_back(mono);
    } else {
      mps::qmatrix[key] = coeff;
    }
  }
}

void qp_solver::parse_quadobj() {
  std::string var1, var2, val;
  assert(mps::pop_token(var1));
  assert(mps::pop_token(var2));
  assert(var1 != "\n" && var2 != "\n");
  assert(mps::pop_token(val));
  assert(val != "\n");

  int var_idx1 = register_var(var1), var_idx2 = register_var(var2);
  Float coeff = std::stold(val);
  if (!is_minimize) coeff = -coeff;

  if (var_idx1 == var_idx2) {
    coeff /= 2;

    _vars[var_idx1].obj_quadratic_coeff += coeff;
    _vars[var_idx1].is_in_obj = true;
    _vars_in_obj.insert(var_idx1);
    _vars[var_idx1].obj_monomials->push_back(_object_monoials.size());
    monomial mono(var_idx1, coeff, false);
    _object_monoials.push_back(mono);
  } else {
    _vars[var_idx1].is_in_obj = true;
    _vars[var_idx2].is_in_obj = true;
    _vars[var_idx1].obj_linear_coeff->push_back(var_idx2);
    _vars[var_idx1].obj_linear_constant_coeff->push_back(coeff);
    _vars[var_idx2].obj_linear_coeff->push_back(var_idx1);
    _vars[var_idx2].obj_linear_constant_coeff->push_back(coeff);
    _vars_in_obj.insert(var_idx1);
    _vars_in_obj.insert(var_idx2);

    _vars[var_idx1].obj_monomials->push_back(_object_monoials.size());
    _vars[var_idx2].obj_monomials->push_back(_object_monoials.size());
    monomial mono(var_idx1, var_idx2, coeff, false);
    _object_monoials.push_back(mono);
  }
}

void qp_solver::parse_mps(char *filename) {
  mps::read_files(filename);

  bool line_begin = true;
  std::string token, state = "";
  while (mps::next_token(token)) {
    if (token == "*" && line_begin) {
      while (mps::pop_token(token)) {
        if (token == "\n") break;
      }
      continue;
    } else if (token == "\n") {
      mps::pop_token();
      line_begin = true;
      continue;
    } else {
      line_begin = false;
    }
    if (token == "NAME") {
      assert(mps::pop_token() && mps::pop_token());
      continue;
    } else if (token == "OBJSENSE") {
      mps::pop_token();
      assert(mps::pop_token(token));
      if (token == "MAX") {
        is_minimize = false;
      } else {
        assert(token == "MIN");
      }
      continue;
    }

    if (token == "ROWS") {
      state = "ROWS";
      mps::pop_token();
    } else if (token == "COLUMNS") {
      state = "COLUMNS";
      mps::pop_token();
    } else if (token == "RHS") {
      state = "RHS";
      mps::pop_token();
    } else if (token == "BOUNDS") {
      state = "BOUNDS";
      mps::pop_token();
    } else if (token == "QCMATRIX") {
      mps::qcmatrix.clear();
      state = "QCMATRIX";
      mps::pop_token();
      is_cons_quadratic = true;

      std::string cons;
      assert(mps::pop_token(cons));
      assert(cons != "\n");
      assert(mps::cons_map.count(cons) > 0);
      mps::qc_num = mps::cons_map[cons];
      _constraints[mps::qc_num].is_quadratic = true;
    } else if (token == "QMATRIX") {
      mps::qmatrix.clear();
      state = "QMATRIX";
      mps::pop_token();
      is_obj_quadratic = true;
    } else if (token == "QUADOBJ") {
      state = "QUADOBJ";
      mps::pop_token();
      is_obj_quadratic = true;
    } else if (token == "ENDATA") {
      state = "ENDATA";
      mps::pop_token();
      break;
    } else {
      if (state == "ROWS") {
        parse_rows();
      } else if (state == "COLUMNS") {
        parse_columns();
      } else if (state == "RHS") {
        parse_rhs();
      } else if (state == "BOUNDS") {
        parse_bounds();
      } else if (state == "QCMATRIX") {
        parse_qcmatrix();
      } else if (state == "QMATRIX") {
        parse_qmatrix();
      } else if (state == "QUADOBJ") {
        parse_quadobj();
      } else {
        std::cerr << "Error: unknown state " << state << std::endl;
        exit(1);
      }
    }
  }
  assert(state == "ENDATA");

  std::vector<std::string>().swap(mps::tokens);
  std::map<std::string, int>().swap(mps::cons_map);
  std::unordered_set<int>().swap(mps::noinit_var);
  std::unordered_set<int>().swap(mps::ulower_var);
  std::map<std::pair<int, int>, Float>().swap(mps::qmatrix);
  std::map<std::pair<int, int>, Float>().swap(mps::qcmatrix);

  for (int var_pos : _bool_vars) {
    auto bin_var = &(_vars[var_pos]);
    for (int con_size : *bin_var->constraints) {
      auto pcon = &(_constraints[con_size]);
      pcon->p_bin_vars.push_back(var_pos);
    }
  }
}

}  // namespace solver

/*
mps读取
0：OBJSENSE MAX或者MIN, 有max的话所有的所有目标函数的变量系数都×-1

1.rows：写了目标函数的名称以及约束的种类

2.col：写了各个变量所在的线性项的目标函数和约束的系数
有整数标志MARK0000  ‘MARKER’                 ‘INTORG’和MARK0001  ‘MARKER’
‘INTEND’标明了整数变量有哪些

3.RHS：写了各个约束的bound值
 注意有rhs       obj     25，表示目标函数有个常数项-25
rhs这个可以是别的例如rhs1之类的，但是后面的约束是固定的

4.bounds: 写了各个变量的上下界，变量默认有下界0
01的就BV，还有free的和inf的都是没有值的，别的是有值的
UP BOUND     x1                  40
，bound这个字符可以换成别的比如BND1，但是前面那个UP等是严格要求的

5.quadobj或者QMATRIX：变量所在目标函数的二次项，记得和lp的一样都得最后除以2
multilinear项：quadobj输的是上三角，注意一下平方的最后还得除以2，qmatrix是重复的，所有的都得除以2
看官方文档

6. QCMATRIX：变量所在约束的二次项
正常加就可以，不需要考虑除以的情况，加起来就对了，看例子11

易错点：问问lp
*/