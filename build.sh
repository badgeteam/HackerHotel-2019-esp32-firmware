#!/bin/bash
bash submodules.sh
bash set_env.sh
bash mpy_cross.sh
rm build -rf
make clean
make
