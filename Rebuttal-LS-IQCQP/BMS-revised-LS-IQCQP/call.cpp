#include <stdio.h>
#include "sol.h"
int main(int argc,char *argv[]) // pri
{
    int flag = 2;
    if (flag == 0)
    {
        solver::qp_solver * ls_qp_solver = new solver::qp_solver;
        //读入文件和时间参数
        ls_qp_solver->read(argv[2]);
        ls_qp_solver->_cut_off = std::atof(argv[1]);
        ls_qp_solver->print_lp_formula(true);
        // ls_qp_solver->print_lp_formula(true);
        exit(0);//第一次
    }
    else if (flag == 1)
    {
        solver::qp_solver * ls_qp_solver = new solver::qp_solver;
        //读入文件和时间参数
        ls_qp_solver->read(argv[2]);
        ls_qp_solver->_cut_off = std::atof(argv[1]);
        // ls_qp_solver->seed_num = std::atoi(argv[3]);
        //读入初始解
        ls_qp_solver->read_init_solution(argv[3]); // 接收lp全文格式的初始解
        // exit(0);
        ls_qp_solver->local_search();
        // // cout << endl;
        // freopen(argv[5], "w", stdout);
        // ls_qp_solver->print_best_solution();
        // fclose(stdout);
        // ls_qp_solver->print_best_solution();
    }
    else if (flag == 2)
    {
        solver::qp_solver * ls_qp_solver = new solver::qp_solver;
        ls_qp_solver->read(argv[2]);
        ls_qp_solver->_cut_off = std::atof(argv[1]);
        ls_qp_solver->print_type = std::atoi(argv[3]);
        // std::cout << "print_type: " << std::atoi(argv[3]) << " " << ls_qp_solver->print_type << std::endl;
        // ls_qp_solver->print_formula();
        ls_qp_solver->local_search();
        // ls_qp_solver->print_best_solution();
    }
}

/*
1.先把等式文件导出出来
2.scip读入文件跑10s，50？

// tar -xvzf scipoptsuite-9.2.0.tgz




3.ls接受，跑剩下的时间50，250？

*/