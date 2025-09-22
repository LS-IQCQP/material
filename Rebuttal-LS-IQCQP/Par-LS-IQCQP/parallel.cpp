#include "parallel.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <iomanip>
#include <iostream>
#include <fmt/format.h>
using namespace std;

ParallelSolver::ParallelSolver(
    char *filename,
    std::string result_path,
    double time_limit,
    int num_threads)
    : time_limit_(time_limit),
      num_threads_(num_threads),
      filename_(filename),
      result_path_(result_path),
      best_obj_value_(std::numeric_limits<double>::max()),
      ls_start_time(std::chrono::high_resolution_clock::now()),
      best_obj_value_idx_(0),
      output_flag_(false)
{
  // mkdir(result_path_.c_str(), 0777);
  // std::string result_file_path = result_path_ + "/ResultFile";
  // result_file_.open(result_file_path, std::ios::out);

  // struct stat file_stat;
  // if (stat(filename, &file_stat) == 0) {
  //   if (file_stat.st_size > 314572800) {
  //     num_threads_ = std::min(num_threads_, 4);
  //   } else if (file_stat.st_size > 209715200) {
  //     num_threads_ = std::min(num_threads_, 6);
  //   }
  // }
  num_threads_ = std::min(num_threads_, 8);
}

void *WorkerSolve(void *arg)
{
  solver::qp_solver *solver = (solver::qp_solver *)arg;
  // cout << "solver[" << solver->parallel_tid << "] starting local search" << endl;
  solver->local_search();
  // if (solver->parallel_tid ==2) solver->print_formula();
  if (solver->bug_flag) cout << "error " << endl;
  // if (solver->restart_flag) 
  // {
  //   cout << solver->portfolio_mode << "restart " << endl;
  // }
  return nullptr;
}

void ParallelSolver::set_minimize(bool is_minimize)
{
  is_minimize_ = is_minimize;
  if (is_minimize_)
    best_obj_value_ = std::numeric_limits<double>::max();
  else
    best_obj_value_ = std::numeric_limits<double>::min();
  output_flag_ = true;
}

char *ParallelSolver::get_filename()
{
  return filename_;
}

void ParallelSolver::Solve()
{
  std::vector<solver::qp_solver *> solver_(num_threads_);
  std::vector<pthread_t> workerPtr(num_threads_);

  solver::qp_solver *master_solver = new solver::qp_solver;
  // cout << "Master solver parsing MPS file: " << filename_ << endl;
  master_solver->parse_mps(filename_);
  bool is_minimize = master_solver->is_minimize;
  set_minimize(is_minimize);
  // cout << "MPS file parsed successfully" << endl;

  std::vector<int> port_mode = {9, 0, 1, 2, 3, 5, 6, 10};
  for (int i = 0; i < num_threads_; i++)
  {
    solver_[i] = new solver::qp_solver;
    solver_[i]->is_minimize = is_minimize;
    solver_[i]->_vars = master_solver->_vars;
    solver_[i]->_constraints = master_solver->_constraints;
    solver_[i]->_bool_vars = master_solver->_bool_vars;
    solver_[i]->_int_vars = master_solver->_int_vars;
    solver_[i]->_var_num = master_solver->_var_num;
    solver_[i]->_bool_var_num = master_solver->_bool_var_num;
    solver_[i]->_int_var_num = master_solver->_int_var_num;
    solver_[i]->_cons_num = master_solver->_cons_num;
    solver_[i]->_object_monoials = master_solver->_object_monoials;
    solver_[i]->is_obj_quadratic = master_solver->is_obj_quadratic;
    solver_[i]->is_cons_quadratic = master_solver->is_cons_quadratic;
    solver_[i]->_obj_constant = master_solver->_obj_constant;
    solver_[i]->_vars_in_obj = master_solver->_vars_in_obj;
    solver_[i]->_vars_map = master_solver->_vars_map;
    solver_[i]->avg_bound = master_solver->avg_bound;

    solver_[i]->portfolio_mode = port_mode[i];
    solver_[i]->parallel_solver = this;
    solver_[i]->parallel_tid = i;
    solver_[i]->output_mode = 3;
    solver_[i]->_cut_off = time_limit_;

    // cout << "solver[" << i << "] initialized with copied data" << endl;
  }

  delete master_solver;

  // result_file_ << "Walltime_start: " << fmt::format("{:.3f}", ElapsedTime()) << std::endl;
  // result_file_ << "elapsed_walltime_in_seconds" << "       " << "best_objective_value" << std::endl;
  for (int i = 0; i < num_threads_; i++)
    pthread_create(&workerPtr[i], NULL, WorkerSolve, (void *)solver_[i]);

  for (int i = 0; i < num_threads_; i++)
    pthread_join(workerPtr[i], NULL);
  if (master_solver->is_cons_quadratic && master_solver->is_obj_quadratic) cout << "quad_quad" << endl;
  else if (master_solver->is_cons_quadratic) cout << "quad_cons" << endl;
  else if (master_solver->_constraints.size() == 0) cout << "ls_no_cons" << endl;
  else cout << "linear_cons" << endl;
  if (is_minimize_) cout << " best obj min= " << std::fixed << best_obj_value_ << endl;
  else cout << " best obj max= " << std::fixed << best_obj_value_ << endl;
  // for (int i = 0; i < num_threads_; i++)
  // {
  //   if (solver_[i].restart_flag) 
  //   {
  //     cout << "restart" << endl;
  //     break;
  //   }
  // }

  for (int i = 0; i < num_threads_; i++)
  {
    delete solver_[i];
  }

  result_file_.close();
}

void ParallelSolver::update_best_solution(
    double temp_obj_value,
    double temp_obj_constant,
    const std::vector<std::string> &var_name,
    const std::vector<double> &var_value)
{
  boost::mutex::scoped_lock lock(mutexTree_);
  if (!output_flag_)
    return;
  if (is_minimize_)
  {
    double real_obj_value = temp_obj_value + temp_obj_constant;
    if (real_obj_value >= best_obj_value_)
      return;
    best_obj_value_ = real_obj_value;
  }
  else
  {
    double real_obj_value = -temp_obj_value + temp_obj_constant;
    if (real_obj_value <= best_obj_value_)
      return;
    best_obj_value_ = real_obj_value;
  }
  best_obj_value_idx_ += 1;
  // if (is_minimize_) cout << std::fixed << best_obj_value_ << " " << ElapsedTime() << endl;
  // else cout << std::fixed << -best_obj_value_ << " " << ElapsedTime() << endl;
  // 输出解、时间、解文件到指定的路径文件
  // std::string solution_file_path = result_path_ + "/SolutionFile" + std::to_string(best_obj_value_idx_);
  // std::ofstream solution_file(solution_file_path);
  // solution_file << "Variable_name" << "       " << "Variable_value" << std::endl;
  // for (int i = 0; i < var_name.size(); i++)
  //   solution_file << var_name[i] << "       " << fmt::format("{}", var_value[i]) << std::endl;

  // result_file_ << fmt::format("{:.3f}", ElapsedTime()) << "                        " << fmt::format("{}", best_obj_value_) << std::endl;
  // solution_file.close();
  // if (is_minimize_)
  //   cout << fmt::format("{}", ElapsedTime()) << "       " << fmt::format("{}", best_obj_value_) << endl;
  // else
  //   cout << fmt::format("{}", ElapsedTime()) << "       " << fmt::format("{}", -best_obj_value_) << endl;
}