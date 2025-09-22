compile:

g++ ls_read.cpp ls_no_cons.cpp ls_bin.cpp component.cpp ls_mix_not_dis.cpp ls_balance.cpp call.cpp -static -O3 -o LS-IQCQP -lgsl -lgslcblas -lm



use:

./LS-IQCQP cutoff filename