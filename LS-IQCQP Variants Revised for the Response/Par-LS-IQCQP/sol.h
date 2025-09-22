#pragma once

#include "util.h"
#include <set>
#include <map>
#include <deque>
#include <memory>
#include <fstream>
// #define DEBUG
// #define OUTPUT_PROCESS
class ParallelSolver;
namespace solver 
{
    struct var {
        int                                             index;
        Float                                           lower = 0, upper, constant;
        bool                                            has_lower = true, has_upper;
        string                                          name;
        std::shared_ptr<unordered_set<int>>             constraints;
        unordered_map<int, int>                         constraints_score;//不用管
        int                                             obj_score;//不用管
        bool                                            is_bin;
        bool                                            is_int;
        bool                                            is_in_obj;
        bool                                            is_constant;
        bool                                            equal_bound = false;
        int                                             bool_score;//不用管
        Float                                           obj_quadratic_coeff; //constant  初始化为0
        std::shared_ptr<vector<int>>                    obj_linear_coeff; //var index
        std::shared_ptr<vector<Float>>                  obj_linear_constant_coeff; //constant
        Float                                           obj_constant_coeff;  //constant 
        std::shared_ptr<vector<int>>                    obj_monomials;//不用管
        int                                             last_pos_step = -10;//TODO:
        int                                             last_neg_step = -10;
        // CircularQueue                                    * recent_value;
        var(int o_index, string o_name) : index(o_index), name(o_name)
        {
            constraints = std::make_shared<unordered_set<int>>();
            obj_linear_coeff = std::make_shared<vector<int>>();
            obj_linear_constant_coeff = std::make_shared<vector<Float>>();
            obj_monomials = std::make_shared<vector<int>>();
            has_lower = true;
            has_upper = false;
            is_constant = false;
            is_bin = false;
            is_int = false;
            obj_constant_coeff = 0;
            obj_quadratic_coeff = 0;
            // recent_value = new CircularQueue;
        }
    };

    struct monomial {
        vector<int>             m_vars;
        // vector<int>             exponent;
        Float                   coeff;
        Float                   value; // the value of this monomial，没有去更新，没用
        bool                    is_linear; //TODO:再想想有没有别的办法区别
        bool                    is_multilinear;
        monomial(int o_var, Float o_coeff, bool o_is_linear) 
        {
            m_vars.push_back(o_var);
            coeff = o_coeff;
            is_linear = o_is_linear;
            is_multilinear = false;
        }
        monomial(int o_var_1, int o_var_2, Float o_coeff, bool o_is_linear) 
        {
            m_vars.push_back(o_var_1);
            m_vars.push_back(o_var_2);
            coeff = o_coeff;
            is_linear = o_is_linear;
            is_multilinear = true;
        }
    };

    struct all_coeff {
        Float                     obj_quadratic_coeff; //constant  初始化为0
        vector<int>               obj_linear_coeff; //var index
        vector<Float>             obj_linear_constant_coeff; //constant
        Float                     obj_constant_coeff;  //constant
        all_coeff()
        {
            obj_quadratic_coeff = 0;
            obj_constant_coeff = 0;
        }
    };

    struct polynomial_constraint {
        polynomial_constraint()
        {
            monomials = std::make_shared<vector<monomial>>();
            var_coeff = std::make_shared<unordered_map<int, all_coeff>>();
        }

        std::shared_ptr<vector<monomial>>               monomials;
        Float                                           value;//只需要知道这个就好，不用知道变量的截距变量
        Float                                           bound;
        std::shared_ptr<unordered_map<int, all_coeff>>  var_coeff;//TODO:构建
        vector<int>                                     p_bin_vars;//不用管
        string                                          name;
        int                                             weight = 1;
        int                                             index;
        bool                                            is_sat;
        bool                                            is_equal = false;
        bool                                            is_less = true;
        bool                                            is_quadratic = false;
        bool                                            is_average = true;
        bool                                            is_linear;
        Float                                           sum;
        // vector<int>                                     p_vars;
        // vector<int>                                     p_quadratic_coeff; //constant  初始化为0
        // vector<vector<int>>                             p_linear_coeff; //var index
        // vector<int>                                     p_linear_constant_coeff; //constant
        // vector<int>                                     p_constant_coeff;  //constant
        // vector<vector<int>>                             p_coeff_vars;
        // vector<vector<int>>                             p_coeff_vars_exponent;
    };
    
    struct pair_vars{
        int var_1;
        int var_2;
        Float value_1;
        Float value_2;
        pair_vars(int var_idx_1, int var_idx_2)
        {
            var_1 = var_idx_1;
            var_2 = var_idx_2;
            value_1 = INT32_MIN;
            value_2 = INT32_MIN;
        }
        pair_vars(int var_idx_1, int var_idx_2, Float change_value_1, Float change_value_2)
        {
            var_1 = var_idx_1;
            var_2 = var_idx_2;
            value_1 = change_value_1;
            value_2 = change_value_2;
        }
        bool operator==(const pair_vars& other) const 
        {
            bool flag = (var_1 == other.var_2) && (var_2 == other.var_1);
            bool flag_2 = (var_1 == other.var_1) && (var_2 == other.var_2);
            return (flag || flag_2);
        }
    };


    class qp_solver {
    public:
        bool                                            restart_flag = false;
        unsigned int                                    rds_seed = 0;
        int                                             _non_improve_step = 0;
        void                                            restart();
        int                                             self_poly_solve_quadratic(Float a, Float b, Float c, Float * root1, Float * root2);
        std::mt19937                                    rds{0};
        ParallelSolver*                                 parallel_solver = nullptr;
        int                                             parallel_tid;
        vector<string>                                  var_name;
        // ##############################################################
        double                                          _best_time;
        bool                                            bug_flag = false;
        int                                             portfolio_mode = 0;
        int                                             output_mode = 0;
        const Float                                     eb = 1e-6 - 1e-13;                
        // const Float                                     eb = 0;          //ali eb = 0 
        //for mix search
        int                                             unsat_cls_num_with_real;
        int                                             unsat_cls_num_with_bool;
        int                                             unbounded_cls_num_with_real;
        int                                             unbounded_cls_num_with_bool;
        int                                             seed_num;
        //var information
        int                                             cons_num_type; // <= 50 : 0      >50 : 1
        int                                             problem_type;
        // 0:纯布尔  1:布尔整数实数 2:布尔+实数 3：纯整数 4：整数实数 5：纯实数
        int                                             _var_num;
        int                                             _bool_var_num;
        int                                             _int_var_num;
        vector<var>                                     _vars;
        unordered_set<int>                              _bool_vars;//TODO:算分 改成size好一点
        unordered_set<int>                              _int_vars;
        unordered_map<string, int>                      _vars_map;
        vector<Float>                                   _cur_assignment;
        vector<Float>                                   _cur_delta;
        unordered_set<int>                              _vars_in_obj;  
        unordered_set<int>                              _obj_vars_in_unbounded_constraint;      
        unordered_set<int>                              _obj_bin_vars_in_unbounded_constraint;                                    
        //cons information 
        int                                             _cons_num = -1;
        vector<polynomial_constraint>                   _constraints;
        unordered_set<int>                              _unsat_constraints;
        unordered_set<int>                              _unbounded_constraints;
        int                                             _best_unsat_num;
        //obj information
        bool                                            is_minimize = true;
        Float                                           _obj_constant = 0;
        vector<monomial>                                _object_monoials;
        vector<int>                                     _object_weights;
        int                                             _object_weight;
        Float                                           _best_object_value = INT64_MAX;
        bool                                            is_obj_quadratic = false;
        bool                                            is_cons_quadratic = false;
        //solution information
        bool                                            print_flag = false;
        int                                             _best_steps = 0;
        vector<Float>                                   _best_assignment;
        bool                                            is_feasible;
        bool                                            is_cur_feasible;
        //selection information
        vector<int>                                     _operation_vars;
        vector<Float>                                   _operation_value;
        vector<int>                                     _operation_vars_sub;// bin vars
        vector<Float>                                   _operation_value_sub; //bin value
        vector<pair_vars>                               _operation_vars_pair;//bin pair vars;
        vector<int>                                     _rand_op_vars;
        vector<Float>                                   _rand_op_values;
        int                                             bms = 100;// 100 200
        const int                                       rand_num = 3;
        const int                                       rand_num_obj = 3;
        //ls information
        int                                             int_problem; // 0 means all real, 1 means mix, 2 means all int
        int                                             bin_problem; // 0 means all real, 1 means mix, 2 means all bool                                           
        int                                             _steps;
        const long long                                 _max_steps = INT64_MAX;   
        std::chrono::steady_clock::time_point           _start_time;
        double                                          _cut_off = 300;
        //constant          
        Float                                           avg_bound = 0;
        //functions
        // bool                                            error_judge
        Float                                           pro_var_value_delta_in_obj_cy(var * nor_var, int var_pos, Float old_value, Float new_value);
        Float                                           calculate_score_cy(int var_idx, Float change_value);
        Float                                           calculate_score_cy_mix(int var_idx, Float change_value);
        bool                                            is_true_number(string str);
        //new
        //read func
        bool                                            bound_flag = false;
        void                                            split_string(string in_string, vector<std::string> &str_vec, string pattern);
        bool                                            isNumber(string str);
        Float                                           pro_coeff(string s_coeff);
        int                                             register_var(string s_var);
        void                                            read(char * filename);
        void                                            read_obj(string line);
        bool                                            with_quadratic = false;
        void                                            read_cons(string line);
        void                                            read_bounds(string line);
        void                                            read_int(string line);
        void                                            read_bin(string line);
        void                                            judge_problem();
        void                                            initialize();
        void                                            initialize_without_cons();
        void                                            initialize_mix();
        double                                          TimeElapsed();
        void                                            precess_small_ins();
        Float                                           pro_mono(monomial mono);
        Float                                           pro_mono_inc(monomial mono, int var_pos);
        void                                            pro_con(polynomial_constraint * pcon);
        void                                            pro_con_mix(polynomial_constraint * pcon);
        Float                                           pro_var_delta(var * bin_var, polynomial_constraint * pcon, int var_pos, bool is_pos);//bool
        Float                                           pro_var_value_delta(var * nor_var, polynomial_constraint * pcon, int var_pos, Float old_value, Float new_value);//int and real
        int                                             pro_var_delta_in_obj(var * bin_var, int var_pos, bool is_pos);
        int                                             pro_var_value_delta_in_obj(var * nor_var, int var_pos, Float old_value, Float new_value);
        Float                                           pro_var_delta_in_obj_cy(var * bin_var, int var_pos, bool is_pos);
        void                                            init_pro_con(polynomial_constraint * pcon);
        void                                            init_pro_con_mix(polynomial_constraint * pcon);
        int                                             judge_cons_state(polynomial_constraint * pcon, Float con_delta);//-1: sat->unsat  0: no change 1: unsat->sat 
        int                                             judge_cons_state_bin(polynomial_constraint * pcon, Float var_delta, Float con_delta);//-1: sat->unsat, unsat->more unsat; 0: no change or sat ->sat 1:unsat -> sat ,more unsat->unsat
        Float                                           judge_cons_state_bin_cy(polynomial_constraint * pcon, Float var_delta, Float con_delta);
        Float                                           judge_cons_state_mix(polynomial_constraint * pcon, Float var_delta, Float con_delta);//-1: sat->unsat  0: no change 1: unsat->sat 
        Float                                           judge_cons_state_bin_cy_mix(polynomial_constraint * pcon, Float var_delta, Float con_delta);
        bool                                            judge_bin_var_feasible(int var_idx, all_coeff * a_coeff, polynomial_constraint * pcon);
        //unsat state
        void                                            insert_operation_unsat();
        void                                            insert_operation_unsat_bin();
        void                                            insert_operation_unsat_mix();
        void                                            insert_operation_unsat_mix_not_dis();
        void                                            insert_var_change_value(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, bool rand_flag);
        void                                            insert_var_change_value_bin(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, bool rand_flag);
        void                                            random_walk_unsat();
        void                                            random_walk_unsat_bin();
        void                                            random_walk_unsat_mix(bool bin_flag);
        void                                            random_walk_unsat_mix_not_dis();
        void                                            no_operation_walk_unsat();
        //sat state
        void                                            insert_operation_sat();
        void                                            insert_operation_sat_bin();
        void                                            insert_operation_sat_bin_with_equal();
        void                                            insert_operation_sat_mix();
        void                                            insert_operation_sat_mix_not_dis();
        // void                                            insert_operation_sat_mix()
        void                                            insert_operation_no_cons();
        void                                            insert_var_change_value_sat(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, int symflag, Float symvalue, bool rand_flag);
        bool                                            insert_var_change_value_sat_bin(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, Float symvalue, bool rand_flag);
        bool                                            insert_var_change_value_sat_bin_equal(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, Float symvalue, bool rand_flag);
        void                                            random_walk_sat();
        void                                            random_walk_sat_bin();
        void                                            random_walk_sat_bin_with_equal();
        void                                            random_walk_sat_mix(bool bin_flag);
        void                                            random_walk_sat_mix_not_dis();
        void                                            random_walk_no_cons();
        void                                            no_operation_walk_sat(int var_idx);
        void                                            lift_move_op(int var_idx, Float & score);//only for bool, 约束少的时候要用吗还是都用？
        int                                             lift_move();
        bool                                            two_flip_no_cons();
        bool                                            fps_move();
        void                                            no_bound_sat_move();
        //new balance op
        bool                                            insert_var_change_value_balance(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, Float symvalue, bool rand_flag);
        void                                            insert_operation_balance();
        void                                            random_walk_balance();
        void                                            local_search_mix_balance();
        //new compensate operators
        bool                                            compensate_move();
        void                                            insert_var_change_value_comp(int var_pos, Float change_value, int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, bool rand_flag);
        void                                            insert_var_change_value_comp_bin(int var_pos, Float change_value, int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, bool rand_flag);
        Float                                           calculate_score_compensate_cons_mix(int var_idx_1, Float change_value_1, int var_idx_2, Float change_value_2, bool is_distance);
        Float                                           calculate_cons_descent_two_vars_mix(int var_idx_1, Float change_value_1, int var_idx_2, Float change_value_2, polynomial_constraint * pcon);
        Float                                           calculate_obj_descent_two_vars_mix(int var_idx_1, Float change_value_1, int var_idx_2, Float change_value_2);
        void                                            select_best_operation_with_pair_mix(int & var_idx_1, Float & change_value_1, int & var_idx_2, Float & change_value_2, Float & score);
        // Float                                           calculate_var_best_value(var * nor_var, int var_pos, int & change_flag); //1 up, 0 quad, -1 down;                                                    
        //ususal in search
        Float                                           calculate_cons_descent_two_vars(int var_idx_1, int var_idx_2, polynomial_constraint * pcon);
        Float                                           calculate_obj_descent_two_vars(int var_idx_1, int var_idx_2);
        Float                                           calculate_score_compensate_cons(int var_idx_1, int var_idx_2);
        Float                                           calculate_coeff_in_obj(int var_idx);
        Float                                           calculate_score(int var_idx, Float change_value);
        Float                                           calculate_score_bin(int var_idx, Float change_value);
        Float                                           calculate_score_bin_cy(int var_idx, Float change_value);
        Float                                           calculate_score_no_cons(int var_idx, Float change_value);//no meas a few ir 0
        Float                                           calculate_score_mix(int var_idx, Float change_value);
        Float                                           is_lift(int var_idx, Float change_value);
        Float                                           obj_lift(int var_idx, Float change_value);
        bool                                            is_cons_lift(polynomial_constraint * pcon, Float var_delta, Float con_delta);
        void                                            select_best_operation(int & var_pos, Float & change_value, Float & score);
        void                                            select_best_operation_bin(int & var_pos, Float & change_value, Float & score);
        void                                            select_best_operation_bin_with_pair(int & var_pos_1, int & var_pos_2, Float & change_value, Float & score);
        void                                            select_best_operation_no_cons(int & var_pos, Float & change_value, Float & score);
        void                                            select_best_operation_mix(int & var_pos, Float & change_value, Float & score);
        void                                            select_best_operation_lift(int & var_pos, Float & change_value, Float & score);
        bool                                            check_var_shift(int var_pos, Float & change_value, bool rand_flag);
        // bool                                            check_var_shift(int var_pos, double & change_value, bool rand_flag);
        bool                                            check_var_shift_bool(int var_pos, Float & change_value, bool rand_flag);
        void                                            execute_critical_move(int var_pos, Float change_value);
        void                                            execute_critical_move_no_cons(int var_pos, Float change_value);
        void                                            execute_critical_move_mix(int var_pos, Float change_value);
        void                                            update_weight();
        void                                            update_weight_no_cons();
        void                                            update_best_solution();
        // Float                                           up_down_float(Float symvalue);
        void                                            local_search_with_real(); //real may exist integer
        void                                            local_search_bin(); //bin only , for qcp
        void                                            local_search_bin_new(); //bin only with equal  for qp
        void                                            local_search_mix();//real bin may exists integer
        void                                            local_search_mix_not_dis();
        void                                            local_search_without_cons();
        void                                            local_search();
        //cout 
        int                                             sta_mix_cons();
        void                                            print_formula();
        void                                            print_cons(int cons_idx);
        void                                            print_mono(monomial mono);
        void                                            print_imp_info();
        void                                            print_best_solution();
        void                                            print_lp_formula(bool is_all_obj);
        // bool                                            print_lp_cons(int cons_idx, bool is_first, bool & is_quad);
        bool                                            print_lp_mono(monomial mono, bool is_first, bool & is_quad, bool is_obj);
        int                                             read_init_solution(char * filename);
        unordered_set<int>                              print_vars; 
        std::vector<monomial>                           print_bounded_objs;
        unordered_map<int, Float>                       _init_solution_map;  
        void                                            restart_by_new_solution();
        void                                            print_bounded_obj();  
        void                                            select_bounded_mono();   
        void                                            add_bool_bound();                 
        void                                            sta_cons();
        // propagation
        class propagation;
        friend class propagation;
        std::unique_ptr<propagation>                    _propagationer;
        void                                            propagate_init(int limit, uint64_t seed, uint8_t type, bool run_linear = false);
        void                                            propagate_fix();
        std::deque<std::pair<int, Float>>               _prop_pool;
        void                                            propagate_move_unsat_mix(int pool_size, int limit_step, Float score_ratio, Float reason_ratio, bool impact_que, bool greedy_prop, bool random_prop, bool positive_score);
        void                                            propagate_init_unsat_mix(int pool_size, bool impact_que);
        void                                            propagate_pick_var_unsat_mix(int pool_size, std::vector<std::tuple<int, Float, Float>> &pool, std::vector<std::tuple<int, Float, Float>> &selects, Float score_ratio, Float reason_ratio);
        bool                                            propagate_selects_unsat_mix(int pool_size, int limit_step, std::vector<std::tuple<int, Float, Float>> &selects, bool impact_que, Float score_ratio, Float reason_ratio);
        void                                            propagate_select_operation_mix(std::vector<std::tuple<int, Float, Float>> &selects, bool positive_score);
        void                                            propagate_random_selects_unsat_mix(std::vector<std::tuple<int, Float, Float>> &selects, bool positive_score);
        // parser
        void                                            parse_mps(char* filename);
        void                                            parse_rows();
        void                                            parse_columns();
        void                                            parse_rhs();
        void                                            parse_bounds();
        void                                            parse_qcmatrix();
        void                                            parse_qmatrix();
        void                                            parse_quadobj();
        // preprocessing
        bool                                            has_assignment = false;
        bool                                            init_assignment = false;
        bool                                            ls_assignment = false;
        int                                             _union_size = 0;
        std::vector<int>                                _union_label = {};
        std::vector<std::set<int>>                      _union_cons = {};
        std::vector<std::vector<int>>                   _union_vars = {};
        std::vector<bool>                               _removed_cons = {};
        std::vector<std::vector<std::pair<int, Float>>> _vars_formula = {};
        void                                            preprocess();
        void                                            copy_init(qp_solver* solver);
        void                                            union_vars();
        void                                            calc_freevars(int idx);
        void                                            add_monomials(polynomial_constraint &con, std::map<std::pair<int, int>, Float> &mono_map);
        void                                            add_constraint(qp_solver* solver, polynomial_constraint &con);
        polynomial_constraint                           calc_cons(const polynomial_constraint &con, bool all_replace);
        void                                            update_cons(qp_solver* solver, bool all_replace);
        qp_solver*                                      preprocessing(int mat_limit, bool all_replace, bool init_flag, bool ls_flag);
        void                                            adjust_assignments();
        // new preprocessing
        qp_solver*                                      preprocessing_new(int mat_limit);
        void                                            update_cons_new(qp_solver* solver);
        // attenuation
        bool                                            attenuation_flag = false;
        void                                            attenuation(int var_idx, Float &change_value);
        // gauss adjust
        bool                                            terminate = false;
        bool                                            open_terminate = false;
        std::shared_ptr<qp_solver>                      adjust_solver = nullptr;
        bool                                            gauss_adjust_flag = false;
        void                                            gauss_adjust_init(qp_solver *init_solver);
        Float                                           var_value_delta_in_obj(int var_idx, Float shift_value);
    };
}
