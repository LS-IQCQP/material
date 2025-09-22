#pragma once

#include "sol.h"
#include "propagation.h"
#include <vector>
#include <thread>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <mutex>
#include <sys/stat.h>
#include <sys/types.h>
#include <atomic>
#include <fmt/format.h>
class solver::qp_solver;

// 函数声明
void *WorkerSolve(void *arg);

class ParallelSolver
{
public:
  ParallelSolver(char *filename, std::string result_path, double time_limit, int num_threads);
  void Solve();
  void update_best_solution(
      double obj_value,
      double obj_constant,
      const std::vector<std::string> &var_name,
      const std::vector<double> &var_value);
  void set_minimize(bool is_minimize);
  char *get_filename();
  void get_best_obj()
  {if (best_obj_value_ != std::numeric_limits<double>::max() && best_obj_value_ != std::numeric_limits<double>::min()) std::cout << fmt::format("{}", best_obj_value_) << endl;
   else std::cout << INT64_MAX;}
private:
  std::atomic<bool> output_flag_;
  std::ofstream result_file_;
  std::string result_path_;
  int best_obj_value_idx_;
  double time_limit_;
  bool is_minimize_;
  int num_threads_;
  char *filename_;
  double best_obj_value_;
  mutable boost::mutex mutexTree_;
  std::chrono::high_resolution_clock::time_point ls_start_time;
  double ElapsedTime()
  {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - ls_start_time).count() / 1000000.0;
  }
};
