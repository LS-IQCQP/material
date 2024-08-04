#pragma once

#include "util.h"
/*TODO:
 0.Float                           value;//只需要知道这个就好，不用知道变量的截距变量 确定吗？
 1.vector 的初始化为0
 2.register var 变量是什么变量?
 3.是要lazy构建还是on the fly构建，还是initial构建系数
 4.TODO 两种情况 ^ *, obj const 要有 ,[] 是不是/2  是 ，明天把约束读入也写完
 5.用set还是vector
 

 915:
 1.看看实数的约束，实数的系数会不会让变量实际上没有让约束满足，.因为一些精度上的损失，所以需不需要上高精度运算，比如/2 就破坏了精度？或者别的情况，比如1/3这种
 2.调研一下高精度运算库 以及有没有比long double更好的
 3. 写到这里了//区分是哪个约束，想好构建逻辑，想好<= >=和bound，别的地方和obj是一样的
 4. 调研看一下300s能超过的有多少个
 5. 写好read能够把multilinear做好
 919:
 split 前面如果有空格对不对，或者有两个空格？
 处理constant的变量
 封装一下构建变量的系数多项式环节
 写到initial了
 920:
 想一下这么设计的数据结构合理吗和NIA相比
 目标函数是不是也得组织一个monomial, 是的，需要！
 minimize 和maximize没有区分啊
  pos 不对，顺序不对，
  cons 处理了一下
  初始化分数
  init写完了
 TODO:
 想一下这么设计的数据结构合理吗和NIA相比
 变量直接等于a的这种如不出现在目标函数里，那么得=处理一下，目前没写这个逻辑
 算bool分的时候写两套逻辑，一套是现算，一套是增量式算，看哪种合适
 all_coeff 改成float的了，看一下有没有应该是float的，但是是int的
 看看有没有<号 没有等号的情况
 想一下init还有没有别的东西需要写
 9.21：
 TODO:
 整数要有一个函数限定为整数，要不会有double的错误？
 多目标，无约束的
 bin 的good_var 需不需要维护，维护的话怎么select
 check var shift 还没有用到，想想什么时候用
 系数的值应该不用时刻更新
 bin var单独设计一套系统,算分需要这么算吗？可能得设计一套var_delta的系统
 变量所在的约束应该是用set，想一下还有没有重复插入没考虑的情况,看一下修复好没有
 bin var的score肯定不能用+1 -1 去负责，因为很容易移动一个变量都造不成+1 -1 尤其是初始的时候！！，所以初始的时候可以不+1 -1; 后来+1 -1;
 bin var如果用增量式score，那么update那里复杂度好高
 mono 没用到除了目标函数？ 需不需要增量更新mono的值
9.22：
TODO:
 sat的情况没做,no operation 的情况没做
 insert 和 check_var_shift 没做
 bin的定制化没做：比如insert的时候不只得checkvar，还得考虑二进制是0 1 移动的，不能是直接让他等于约束满足.var的score肯定不能用+1 -1 去负责，因为很容易移动一个变量都造不成+1 -1 尤其是初始的时候！！，所以初始的时候可以不+1 -1; 后来+1 -1;
 看一下计算分数，移动的时候，赋值改变的情况对不对的问题。看一下加权的逻辑，看一下pro_delta四个函数写的对不对。
  minimize 和maximize别忘了最后的时候区分一下,objconst怎么处理的
9.23
TODO:
维护一个obj变量的unbounded列表，这个之后再写吧
set 不能随机访问，看看有没有那种把数组下标输出，而不是数组元素的情况存在
没有处理好bound是不是约束这一点
9.24
TODO:
no operation 的sat和unsat都没做
insert 和 check_var_shift 没做  分为obj的和普通约束的
二进制定制化没做，上述 
bound是不是约束这点没做，要把它加入insert里面，上下界变量相等，下边界是0,初始化为下边界，然后每次在边界里移动就行，不知道有没有<号而不是<=号
除的系数如果很小怎么办？
score 重新定义一下，二进制变量或者obj的score都得再权衡一下
tabu还没做
最后总体读一遍
在seed上配置gsl
random_walk_sat 和unsat 的情况都可以通过数据机构记录一下变量和change_value，避免重复计算。sat的根据变量选择几个操作，unsat的根据约束选择几个操作，这个之后的版本再写吧
9.25:
ax的系数是0的情况
因为目标函数是二次的，所以insert_var_obj是要跟坐标轴挂钩的
想一下当时smt里那个bug是啥来，nia理论的！！
精度问题会不会导致约束未满足但是搞得满足的情况 
PPT和WWW修改
tabu没有设置好
想一下github账户是什么来
9.29:
想一下二次方程无解的情况会怎么样，在贪心步的时候
10.11：
<=号怎么安排
相等变量没有安排
明天写sat的情况和no operation 的sat和unsat都没做
后天写bool的情况
10.12:
tacas
写sat的情况和no operation 的sat和unsat都没做
看一下是不是权重重启的对了
10.18:
弄一个unbounded的obj cons ,取样技术也没有加入
random walk和非random walk之间可以记录一下,random walke里的obj var取样了，但是objvar的约束没有取样
变量边界要插入进去，想想怎么插入，sat和unsat的，check_var_shift那里应该放在之前
二次项增量式计算的时候有没有出错？不应该是a(x1-x2)^2,而是ax1^2 -ax^2,
应该用changevalue还是直接就把值算出来？
无约束的优化？
bin sat no operation

10.20
想一下还没做的：
matters！！！！
1.sat 二次的情况 {
    1.只有一个解
    2.两个解的情况
}
一次的情况整数的情况还没有解决,没有解的情况整数的问题没有解决 //done 
整数问题sat的 //done 
//TODO: check 这种情况
7.变量边界要插入进去，想想怎么插入，sat和unsat的，check_var_shift那里应该放在之前

2.no op的情况 //done
//TODO: check 做的对不对，
no op unsat应该可以和random walk合并一下，就像random walk和insert合并一下一样，sat的情况也得合并
想一下放的逻辑，unsat目前是只放入一个未满足约束的变量，是不是可以放入的更多，它的移动范围是不是也不一定是+1 -1，比如根据变量所在的约束或者万一变量是obj变量，sat的情况是插入一个目标函数中的变量中，随机往上或者往下走1，是不是根据二次的开口或者线性的开口给？
而且这种情况确实还是会有no op的情况，那么这就是算法的问题了，得再好好改改算法

9.相等变量没有安排，变量上下界相等 //done
TODO: 没有进行化简,会不会出现两个约束里 a<=5 a>=5的情况，这种情况不会出错，但是没有放入到equal bound var里之后化简可能得用到

3.bin，怎么和实数整数变量区分，select的时候,bin和整数的变量需要把都取到上界的终止条件加入
纯布尔的一套做法，有布尔变量的一套做法？
4.无约束的情况

little mattes:
5.实数的精度问题没用解决，double和long double之间的
6.判断实数的问题的时候要让他往上走还是往下走的情况，实数的系数会不会让变量实际上没有让约束满足，.因为一些精度上的损失，所以需不需要上高精度运算，比如/2 就破坏了精度？或者别的情况，比如1/3这种
8.<号怎么安排
10.初始化变量的时候没用考虑整数？// done

应该要改的: should do!
1.应该用changevalue还是直接就把值算出来？
2.二次项增量式计算的时候有没有出错？不应该是a(x1-x2)^2,而是ax1^2 -ax^2,
3.弄一个unbounded的obj cons vars ,取样技术也没有加入
4.random walk和非random walk之间可以记录一下,random walke里的obj var取样了，但是objvar的约束没有取样
5.看一下是不是权重重启的对了
6.minimize 和maximize别忘了最后的时候区分一下,objconst怎么处理的
7.tabu检查一下
8.把逻辑都改成<=
9.check 一下check_var shift 
10. check 一下变量var的这个vector是不是都对应的上
11 fabsl 和fabs？
12. 需不需要把对称轴这个操作加入？感觉是需要的
13. /2  check
14.只通过变量的类型是不是不能完全判断问题类型，有可能通过bound+整数限制了他是布尔？
整体check一遍*/ 

// smt nia idea
// 弥补操作加进去
// 将一个约束中的几个变量同时下降使得约束满足
// 目标函数中的变量按照梯度和可行域下降
// 维护硬子句不动，对软子句中所含的变量进行可行域操作，感觉可以后期启用，为了找到最优解去用。{
// 要么就是在硬子句的基础上，找到软子句里能让权重减小的变量操作空间
// 要么就是在硬子句和已有软子句的基础上，找到能使新的未满足的软子句满足的空间    
//}

//check博士有没有确认
// #define DEBUG
namespace solver 
{
    struct var {
        int                     index;
        Float                   lower = 0, upper, constant;
        bool                    has_lower = true, has_upper;
        string                  name;
        unordered_set<int>      constraints;
        unordered_map<int, int> constraints_score;
        int                     obj_score;
        bool                    is_bin;
        bool                    is_int;
        bool                    is_in_obj;
        bool                    is_constant;
        bool                    equal_bound = false;
        int                     bool_score;
        Float                   obj_quadratic_coeff; //constant  初始化为0
        vector<int>             obj_linear_coeff; //var index
        vector<Float>           obj_linear_constant_coeff; //constant
        Float                   obj_constant_coeff;  //constant
        vector<int>             obj_monomials;
        int                     last_pos_step = -10;//TODO:
        int                     last_neg_step = -10;
        var(int o_index, string o_name) : index(o_index), name(o_name)
        {
            has_lower = true;
            has_upper = false;
            is_constant = false;
            is_bin = false;
            is_int = false;
            obj_constant_coeff = 0;
            obj_quadratic_coeff = 0;
        }
    };

    struct monomial {
        vector<int>             m_vars;
        // vector<int>             exponent;
        Float                   coeff;
        Float                   value; // the value of this monomial，没有去更新
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
        vector<monomial>                monomials;
        Float                           value;//只需要知道这个就好，不用知道变量的截距变量
        Float                           bound;
        unordered_map<int, all_coeff>   var_coeff;//TODO:构建
        vector<int>                     p_bin_vars;
        string                          name;
        int                             weight = 1;
        int                             index;
        bool                            is_sat;
        bool                            is_equal = false;
        bool                            is_less = true;
        bool                            is_quadratic = false;
        // vector<int>                     p_vars;
        // vector<int>                     p_quadratic_coeff; //constant  初始化为0
        // vector<vector<int>>             p_linear_coeff; //var index
        // vector<int>                     p_linear_constant_coeff; //constant
        // vector<int>                     p_constant_coeff;  //constant
        // vector<vector<int>>             p_coeff_vars;
        // vector<vector<int>>             p_coeff_vars_exponent;
    };
    
    struct pair_vars{
        int var_1;
        int var_2;
        pair_vars(int var_idx_1, int var_idx_2)
        {
            var_1 = var_idx_1;
            var_2 = var_idx_2;
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
        double                                          _best_time;
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
        int                                             _obj_constant = 0;
        vector<monomial>                                _object_monoials;
        vector<int>                                     _object_weights;
        int                                             _object_weight;
        Float                                           _best_object_value = INT32_MAX;
        bool                                            is_obj_quadratic = false;
        bool                                            is_cons_quadratic = false;
        //solution information
        bool                                            print_flag = false;
        int                                             _best_steps;
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
        const int                                       _max_steps = INT32_MAX;   
        std::chrono::steady_clock::time_point           _start_time;
        double                                          _cut_off = 300;
        //constant          
        Float                                           avg_bound = 0;
        //functions
        Float                                           pro_var_value_delta_in_obj_cy(var * nor_var, int var_pos, Float old_value, Float new_value);
        Float                                           calculate_score_cy(int var_idx, Float change_value);
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
        double                                          TimeElapsed();
        void                                            precess_small_ins();
        Float                                           pro_mono(monomial mono);
        Float                                           pro_mono_inc(monomial mono, int var_pos);
        void                                            pro_con(polynomial_constraint * pcon);
        Float                                           pro_var_delta(var * bin_var, polynomial_constraint * pcon, int var_pos, bool is_pos);//bool
        Float                                           pro_var_value_delta(var * nor_var, polynomial_constraint * pcon, int var_pos, Float old_value, Float new_value);//int and real
        int                                             pro_var_delta_in_obj(var * bin_var, int var_pos, bool is_pos);
        int                                             pro_var_value_delta_in_obj(var * nor_var, int var_pos, Float old_value, Float new_value);
        Float                                           pro_var_delta_in_obj_cy(var * bin_var, int var_pos, bool is_pos);
        void                                            init_pro_con(polynomial_constraint * pcon);
        int                                             judge_cons_state(polynomial_constraint * pcon, Float con_delta);//-1: sat->unsat  0: no change 1: unsat->sat 
        int                                             judge_cons_state_bin(polynomial_constraint * pcon, Float var_delta, Float con_delta);//-1: sat->unsat, unsat->more unsat; 0: no change or sat ->sat 1:unsat -> sat ,more unsat->unsat
        Float                                           judge_cons_state_bin_cy(polynomial_constraint * pcon, Float var_delta, Float con_delta);
        bool                                            judge_bin_var_feasible(int var_idx, all_coeff * a_coeff, polynomial_constraint * pcon);
        //unsat state
        void                                            insert_operation_unsat();
        void                                            insert_operation_unsat_bin();
        void                                            insert_operation_unsat_mix();
        void                                            insert_var_change_value(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, bool rand_flag);
        void                                            insert_var_change_value_bin(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, bool rand_flag);
        void                                            random_walk_unsat();
        void                                            random_walk_unsat_bin();
        void                                            random_walk_unsat_mix(bool bin_flag);
        void                                            no_operation_walk_unsat();
        //sat state
        void                                            insert_operation_sat();
        void                                            insert_operation_sat_bin();
        void                                            insert_operation_sat_bin_with_equal();
        void                                            insert_operation_sat_mix();
        void                                            insert_operation_no_cons();
        void                                            insert_var_change_value_sat(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, int symflag, Float symvalue, bool rand_flag);
        bool                                            insert_var_change_value_sat_bin(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, Float symvalue, bool rand_flag);
        bool                                            insert_var_change_value_sat_bin_equal(int var_idx, all_coeff * a_coeff, Float delta, polynomial_constraint * pcon, Float symvalue, bool rand_flag);
        void                                            random_walk_sat();
        void                                            random_walk_sat_bin();
        void                                            random_walk_sat_bin_with_equal();
        void                                            random_walk_sat_mix(bool bin_flag);
        void                                            random_walk_no_cons();
        void                                            no_operation_walk_sat(int var_idx);
        void                                            lift_move_op(int var_idx, Float & score);//only for bool, 约束少的时候要用吗还是都用？
        int                                             lift_move();
        bool                                            two_flip_no_cons();
        bool                                            fps_move();
        void                                            no_bound_sat_move();
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
        void                                            select_best_operation(int & var_pos, Float & change_value, Float & score);
        void                                            select_best_operation_bin(int & var_pos, Float & change_value, Float & score);
        void                                            select_best_operation_bin_with_pair(int & var_pos_1, int & var_pos_2, Float & change_value, Float & score);
        void                                            select_best_operation_no_cons(int & var_pos, Float & change_value, Float & score);
        bool                                            check_var_shift(int var_pos, Float & change_value, bool rand_flag);
        bool                                            check_var_shift(int var_pos, double & change_value, bool rand_flag);
        bool                                            check_var_shift_bool(int var_pos, Float & change_value, bool rand_flag);
        void                                            execute_critical_move(int var_pos, Float change_value);
        void                                            execute_critical_move_no_cons(int var_pos, Float change_value);
        void                                            update_weight();
        void                                            update_weight_no_cons();
        void                                            update_best_solution();
        // Float                                           up_down_float(Float symvalue);
        void                                            local_search_with_real(); //real may exist integer
        void                                            local_search_bin(); //bin only , for qcp
        void                                            local_search_bin_new(); //bin only with equal  for qp
        void                                            local_search_mix();//real bin may exists integer
        void                                            local_search_without_cons();
        void                                            local_search();
        //cout 
        void                                            print_formula();
        void                                            print_cons(int cons_idx);
        void                                            print_mono(monomial mono);
        void                                            print_imp_info();
        void                                            print_best_solution();
    };
}
