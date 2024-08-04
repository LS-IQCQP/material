#include <stdio.h>
#include "sol.h"
int main(int argc,char *argv[])
{
    solver::qp_solver * ls_qp_solver = new solver::qp_solver;
    ls_qp_solver->read(argv[2]);
    ls_qp_solver->_cut_off = std::atof(argv[1]);
    // ls_qp_solver->seed_num = std::atoi(argv[3]);
    // ls_qp_solver->print_formula();
    // ls_qp_solver->print_imp_info();
    // exit(0);
    ls_qp_solver->local_search();
}