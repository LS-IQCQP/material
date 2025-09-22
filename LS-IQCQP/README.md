# LS-IQCQP: Local Search for Integer Quadratic Constrained Quadratic Programming

## Description

LS-IQCQP is a local search algorithm implementation for solving Integer Quadratic Constrained Quadratic Programming problems. The algorithm supports multiple search strategies including greedy strategy and tabu search strategy.

## Compilation

### Dependencies

Ensure the following libraries are installed on your system:
- GSL (GNU Scientific Library)
- GSL CBLAS
- Standard math library

### Compilation Command

```bash
g++ ls_read.cpp ls_no_cons.cpp ls_bin.cpp component.cpp ls_mix_not_dis.cpp ls_balance.cpp call.cpp -static -O3 -o LS-IQCQP -lgsl -lgslcblas -lm
```

### Compilation Options

- `-static`: Static linking
- `-O3`: Highest level optimization
- `-lgsl -lgslcblas`: Link GSL libraries
- `-lm`: Link math library

## Usage

### Basic Syntax

```bash
./LS-IQCQP <cutoff> <filename> <tabu_flag>
```

### Parameters

- `cutoff`: Time limit in seconds
- `filename`: Input problem file path
- `tabu_flag`: Tabu search strategy flag
  - `0`: Disable tabu search strategy
  - `1`: Enable tabu search strategy

### Examples

```bash
# Disable tabu search strategy with 300 seconds time limit
./LS-IQCQP 300 problem.lp 0

# Enable tabu search strategy with 300 seconds time limit
./LS-IQCQP 300 problem.lp 1
```
