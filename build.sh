#!/bin/bash
bash submodules.sh
source set_env.sh
bash mpy_cross.sh
rm build -rf
make clean
make -j8
