#!/bin/bash
if [[ "$OSTYPE" != "darwin"* ]]; then
	bash submodules.sh
fi
source set_env.sh
bash mpy_cross.sh
rm build -rf
make clean
make -j8
