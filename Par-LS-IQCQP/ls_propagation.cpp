#include "mathutils.h"
#include "propagation.h"
#include <algorithm>
#include <random>

namespace solver {

void qp_solver::propagate_fix() {
  for (int i = 0; i < _constraints.size(); ++i) {
    auto &con = _constraints[i];

    bool is_linear = true;
    int int_num = 0, real_num = 0, int_var = -1;
    for (int j = 0; j < con.monomials->size() && is_linear; ++j) {
      auto &mono = con.monomials->at(j);
      is_linear &= mono.is_linear;

      int var_idx = mono.m_vars[0];
      if (_vars[var_idx].is_int) {
        ++int_num;
        int_var = var_idx;
      } else {
        ++real_num;
      }
    }

    if (is_linear && int_num == 1 && real_num == 2) {
      _propagationer->add_cons_fixvar(i, int_var);

#ifdef INIT_LOG
      fprintf(stderr, "***[propagate_fix]*** constraint %s: fixed vars %s\n", _constraints[i].name.c_str(),
              _vars[int_var].name.c_str());
#endif
    }
  }
}

void qp_solver::propagate_init(int limit, uint64_t seed, uint8_t type, bool run_linear) {
  auto start = std::chrono::steady_clock::now();

  type = type % 3;
  _propagationer = std::make_unique<propagation>(*this, (qp_solver::propagation::range_type)type);
  // propagate_fix();
  return;
  std::vector<std::tuple<polynomial_constraint, int, int>> order_cons;
  std::mt19937 rng(seed);

  for (int i = 0; i < _constraints.size(); i++) {
    // if (!_constraints[i].is_linear) continue;
    bool has_bool = false, has_real = false;
    for (auto &mono : *_constraints[i].monomials) {
      for (auto &var : mono.m_vars) {
        if (_vars[var].is_bin)
          has_bool = true;
        else
          has_real = true;
      }
    }
    int type = has_bool && !has_real ? 0 : (has_real && !has_bool ? 1 : 2);
    order_cons.emplace_back(std::make_tuple(_constraints[i], type, _constraints[i].monomials->size()));
  }
  std::sort(
      order_cons.begin(), order_cons.end(),
      [](const std::tuple<polynomial_constraint, int, int> &a, const std::tuple<polynomial_constraint, int, int> &b) {
        if (std::get<1>(a) != std::get<1>(b))
          return std::get<1>(a) < std::get<1>(b);
        return std::get<2>(a) > std::get<2>(b);
      });

  std::vector<int> var_limit(_vars.size(), limit);
  for (int i = 0; i < order_cons.size(); i++) {
    const auto &[con, type, size] = order_cons[i];
    Float total = 0, bound = con.bound;
    for (auto &mono : *con.monomials)
      total += std::abs(mono.coeff);
    assert(std::abs(total) > eb);

    auto update = [&](int var_idx, all_coeff &coeff) {
      if (_propagationer->var_is_fixed(var_idx))
        return;
      if (var_limit[var_idx] <= 0)
        return;

      --var_limit[var_idx];
      Float select = 0;
      if (_vars[var_idx].is_bin) {
        select = rng() % 2;
      } else {
        select = std::abs(coeff.obj_constant_coeff) + std::abs(coeff.obj_quadratic_coeff);
        for (auto &v : coeff.obj_linear_constant_coeff)
          select += std::abs(v);
        select = bound / total * select;
        if (coeff.obj_constant_coeff < 0)
          select = -select;

        if (_vars[var_idx].is_int)
          select = std::round(select);
      }

      if (!_propagationer->var_in_range(var_idx, select)) {
        if (select > _propagationer->get_lower(var_idx))
          select = _propagationer->get_lower(var_idx);
        else
          select = _propagationer->get_upper(var_idx);
      }
      std::vector<std::pair<int, Float>> quadratic_pair = {{var_idx, select}};
      if (run_linear)
        _propagationer->propagate_linear(quadratic_pair, false, false);
      else
        _propagationer->propagate_quadratic(quadratic_pair, false, false);
    };
    for (auto [var_idx, coeff] : *con.var_coeff)
      if (_vars[var_idx].is_bin)
        update(var_idx, coeff);
    for (auto [var_idx, coeff] : *con.var_coeff)
      if (!_vars[var_idx].is_bin)
        update(var_idx, coeff);
  }

  auto end = std::chrono::steady_clock::now();
  std::cout << "propagation init time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << "ms" << std::endl;
}

void qp_solver::propagate_select_operation_mix(std::vector<std::tuple<int, Float, Float>> &selects,
                                               bool positive_score) {
  int cnt;
  int op_size = _operation_vars.size();
  bool is_bms;
  // bms = 200;
  if (op_size <= bms) {
    cnt = op_size;
    is_bms = false;
  } else {
    cnt = bms;
    is_bms = true;
  }
  int cur_var;
  Float cur_shift, cur_score;
  int rand_index;

  selects.clear();
  for (int i = 0; i < cnt; i++) {
    if (is_bms) {
      rand_index = rds() % (op_size - i);
      cur_var = _operation_vars[rand_index];
      cur_shift = _operation_value[rand_index];
      _operation_vars[rand_index] = _operation_vars[op_size - i - 1];
      _operation_value[rand_index] = _operation_value[op_size - i - 1];
    } else {
      cur_var = _operation_vars[i];
      cur_shift = _operation_value[i];
    }
    if (is_cur_feasible) // 打破faclay80,35的记录，cruoil02的记录，以及
      // if (true) //打破ring的两个记录 ,e-12是最后一个，e的-9是第二个
      // if (is_feasible) // 打破4个faclay记录的版本
      cur_score = calculate_score_cy_mix(cur_var, cur_shift);
    else
      cur_score = calculate_score_mix(cur_var, cur_shift);
    // cur_score = calculate_score(cur_var, cur_shift);
    // var * cur_var_real = &(_vars[cur_var]);
    // if (cur_var_real->recent_value->contains(_cur_assignment[cur_var] + cur_shift) && !is_cur_feasible)
    // {
    //     if (cur_score > 0)
    //         cur_score /= 1.5;
    //     else cur_score *= 1.5;
    // }
    if (!positive_score || cur_score > 0)
      selects.emplace_back(std::make_tuple(cur_var, cur_shift, cur_score));
  }
}

void qp_solver::propagate_random_selects_unsat_mix(std::vector<std::tuple<int, Float, Float>> &selects,
                                                   bool positive_score) {
  polynomial_constraint *unsat_con;
  all_coeff *a_coeff;
  int var_idx;
  Float delta;
  unordered_set<int> rand_unsat_idx;
  _operation_vars_sub.clear();
  _operation_value_sub.clear();
  _operation_vars.clear();
  _operation_value.clear();
  // int real_rand_num = 20;
  for (int i = 0; i < rand_num; i++) {
    unordered_set<int>::iterator it(_unsat_constraints.begin());
    std::advance(it, rds() % _unsat_constraints.size());
    rand_unsat_idx.insert(*it);
  }
  // cout << rand_unsat_idx.size() << endl;
  for (int unsat_pos : rand_unsat_idx) {
    unsat_con = &(_constraints[unsat_pos]);
    delta = unsat_con->bound - unsat_con->value;
    for (auto var_coeff : *unsat_con->var_coeff) {
      var_idx = var_coeff.first;
      a_coeff = &(var_coeff.second);
      // insert_var_change_value_bin(var_idx, a_coeff, delta, unsat_con);
      if (_vars[var_idx].is_bin)
        insert_var_change_value_bin(var_idx, a_coeff, 0, unsat_con, true);
      else if (!_vars[var_idx].is_bin)
        insert_var_change_value(var_idx, a_coeff, delta, unsat_con, true);
    }
  }
  for (int i = 0; i < _operation_vars_sub.size(); i++) {
    // if (_operation_value_sub[i] != 1 && _operation_value_sub[i] != -1) cout <<"??????" <<endl;
    _operation_vars.push_back(_operation_vars_sub[i]);
    _operation_value.push_back(_operation_value_sub[i]);
  }
  // for (int i = 0; i < _operation_vars.size(); i++)
  // {
  //     if (_operation_value[i] == 2 && _vars[_operation_vars[i]].name =="b1") cout << " ??????";
  // }
  // cout << _operation_value.size() << endl;
  propagate_select_operation_mix(selects, positive_score);
}

void qp_solver::propagate_pick_var_unsat_mix(int pool_size, std::vector<std::tuple<int, Float, Float>> &pool,
                                             std::vector<std::tuple<int, Float, Float>> &selects, Float score_ratio,
                                             Float reason_ratio) {
  std::vector<Float> ls_score, reason_score;
  ls_score.reserve(selects.size());
  reason_score.reserve(selects.size());
  for (const auto &[var_idx, shift, score] : selects) {
    ls_score.emplace_back(score);
    reason_score.emplace_back(_propagationer->get_reason(var_idx));
  }
  mathutils::min_max_normalize(ls_score);
  mathutils::min_max_normalize(reason_score);

  std::vector<std::tuple<int, Float, Float>> tmp_pool;
  for (int i = 0; i < selects.size(); i++) {
    // if (_propagationer->var_is_fixed(std::get<0>(selects[i])))
    //   continue;
#ifdef LS_LOG
    fprintf(stderr, "***[propagate_pick_var_unsat_mix]*** var: %s, shift: %Le, ls_score: %Le, reason_score: %Le\n",
            _vars[std::get<0>(selects[i])].name.c_str(), std::get<1>(selects[i]), ls_score[i], reason_score[i]);
#endif
    Float score = ls_score[i] * score_ratio + reason_score[i] * reason_ratio;
    tmp_pool.emplace_back(std::get<0>(selects[i]), std::get<1>(selects[i]), score);
  }

  pool.resize(std::min(pool_size, (int)tmp_pool.size()));
  std::partial_sort_copy(tmp_pool.begin(), tmp_pool.end(), pool.begin(), pool.end(),
                         [](const std::tuple<int, Float, Float> &a, const std::tuple<int, Float, Float> &b) {
                           return std::get<2>(a) > std::get<2>(b);
                         });
}

bool qp_solver::propagate_selects_unsat_mix(int pool_size, int limit_steps,
                                            std::vector<std::tuple<int, Float, Float>> &selects, bool impact_que,
                                            Float score_ratio, Float reason_ratio) {
#ifdef LS_LOG
  fprintf(stderr, "***[propagate_selects_unsat_mix]*** start select\n");
#endif
  std::vector<std::tuple<int, Float, Float>> pool;
  propagate_init_unsat_mix(pool_size, impact_que);
  propagate_pick_var_unsat_mix(limit_steps, pool, selects, score_ratio, reason_ratio);

  bool success = false;
  for (const auto &[var_idx, shift, score] : pool) {
    if ((TimeElapsed() > _cut_off) || terminate) break;
    // std::vector<std::pair<int, Float>> changes;
    _propagationer->init_ranges();
    std::vector<std::pair<int, Float>> changes(_prop_pool.begin(), _prop_pool.end());
    changes.emplace_back(var_idx, _cur_assignment[var_idx] + shift);
#ifdef LS_LOG
    fprintf(stderr, "***[propagate_selects_unsat_mix]*** var: %s, shift: %Le\n", _vars[var_idx].name.c_str(), shift);
#endif
    if (_propagationer->propagate_linear(changes, true, impact_que).size() > 0) {
#ifdef LS_LOG
      fprintf(stderr, "***[propagate_selects_unsat_mix]*** var %s success\n", _vars[var_idx].name.c_str());
      fprintf(stderr, "***[propagate_selects_unsat_mix]*** fixed vars: ");
      for (int i = 0; i < _vars.size(); i++)
        if (_propagationer->var_is_fixed(i))
          fprintf(stderr, "%s ", _vars[i].name.c_str());
      fprintf(stderr, "\n");
#endif
      _prop_pool.push_back(changes.back());
      _propagationer->apply_assignment();
      success = true;
      break;
    }
  }
  if (!success) {
#ifdef LS_LOG
    fprintf(stderr, "***[propagate_selects_unsat_mix]*** all vars failed\n");
#endif
  }
  return success;
}

void qp_solver::propagate_init_unsat_mix(int pool_size, bool impact_que) {
#ifdef LS_LOG
  fprintf(stderr, "***[propagate_init_unsat_mix]*** now prop_pool: ");
  for (const auto &[var_idx, value] : _prop_pool)
    fprintf(stderr, "(%s, %Le) ", _vars[var_idx].name.c_str(), value);
  fprintf(stderr, "\n");
#endif
  if (_prop_pool.size() == pool_size) {
    _prop_pool.pop_front();

#ifdef LS_LOG
    fprintf(stderr, "***[propagate_init_unsat_mix]*** prop_pool is full, pop front\n");
    fprintf(stderr, "***[propagate_init_unsat_mix]*** now prop_pool: ");
    for (const auto &[var_idx, value] : _prop_pool)
      fprintf(stderr, "(%s, %Le) ", _vars[var_idx].name.c_str(), value);
    fprintf(stderr, "\n");
#endif
  }

  _propagationer->init_ranges();
  std::vector<std::pair<int, Float>> changes(_prop_pool.begin(), _prop_pool.end());
  if (_propagationer->propagate_linear(changes, false, impact_que).size() == 0) {
#ifdef LS_LOG
    fprintf(stderr, "***[propagate_init_unsat_mix]*** initialisation failed, clear prop_pool\n");
#endif
    _prop_pool.clear();
  }
}

/*
 * propagate_move_unsat_mix()
 * args: 1. pool_size: 传播时，会把最近的pool_size个变量固定，然后进行传播
 *       2. limit_step: 最多选择limit_step次变量进行传播，如果没有选择到可行解，就会停止
 *       3. score_ratio, reason_ratio: 选择变量时，ls_score分数 和 reason_score分数的加权系数
 *       4. impact_que: 指的是传播过程的option是否开启
 * propagate(a) {
 * for C contain a:
      for x in C:
        lx,rx = cal_bound(x)
        if (rx < lx) return conflict
        if (lx == rx) assignment[x] = lx, queue.push(x);
        if (assignment[x] < lx || assignment[x] > rx)
            assignment[x] = lx 或 rx;
            if (option) queue.push(x);
  *       5. greedy_prop: 在贪心步是否传播
  *       6. random_prop: 在随机步是否传播
  *       7. select时，是否只选择score>0的变量
  *
  * propagate_select_operation_mix(selects, positive_score)
  *   对应的是select_best_operation_mix()
  *   把所有的情况丢到selects里面
  *
  * propagate_selects_unsat_mix(pool_size, limit_step, selects, impact_que, score_ratio, reason_ratio)
  *   就是进行传播，需要把propagate_select_operation_mix得到的selects传进去
  *   返回值表示是否传播成功
  *
  * propagate_random_selects_unsat_mix(selects, positive_score)
  *   是插入+select的操作
 */
void qp_solver::propagate_move_unsat_mix(int pool_size, int limit_step, Float score_ratio, Float reason_ratio,
                                         bool impact_que, bool greedy_prop, bool random_prop, bool positive_score) {
  std::vector<std::tuple<int, Float, Float>> selects;

  auto greedy_step = [&]() -> bool {
    insert_operation_unsat_mix_not_dis();
    if (greedy_prop) {
      propagate_select_operation_mix(selects, positive_score);
      return propagate_selects_unsat_mix(pool_size, limit_step, selects, impact_que, score_ratio, reason_ratio);
    } else {
      int var_pos;
      Float change_value, score;
      select_best_operation_mix(var_pos, change_value, score);
      if (score > 0) {
        execute_critical_move_mix(var_pos, change_value);
        return true;
      } else {
        return false;
      }
    }
  };

  auto random_step = [&]() -> void {
    update_weight();
    if (random_prop) {
      propagate_random_selects_unsat_mix(selects, positive_score);
      if (!propagate_selects_unsat_mix(pool_size, limit_step, selects, impact_que, score_ratio, reason_ratio)) {
        no_operation_walk_unsat();
      }
    } else {
      random_walk_unsat_mix_not_dis();
    }
  };

  if (!greedy_step()) {
#ifdef LS_LOG
    fprintf(stderr, "***[propagate_move_unsat_mix]*** greedy select failed, try to "
                    "random select\n");
#endif
    random_step();
#ifdef LS_LOG
    fprintf(stderr, "***[propagate_move_unsat_mix]*** random select success, now unsat constraints: %d\n",
            (int)_unsat_constraints.size());
#endif
  } else {
#ifdef LS_LOG
    fprintf(stderr, "***[propagate_move_unsat_mix]*** greedy select success, now unsat constraints: %d\n",
            (int)_unsat_constraints.size());
#endif
  }
}

} // namespace solver