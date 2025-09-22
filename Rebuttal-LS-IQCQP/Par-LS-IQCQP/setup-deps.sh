#!/bin/bash
set -e

rm -rf deps
mkdir deps

# Setup fmt
git clone https://github.com/fmtlib/fmt.git deps/fmt
cd deps/fmt
mkdir build
cd build
cmake ..
make -j
cd ../..

# Setup gsl
git clone https://github.com/ampl/gsl.git gsl
cd gsl
mkdir build
cd build
cmake .. -G"Unix Makefiles" -DNO_AMPL_BINDINGS=1
make -j
