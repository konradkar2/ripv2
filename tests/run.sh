#/bin/bash

set -e
cwd=$(pwd)

#run unit tests
cd ./src/build_debug/tests
ctest --verbose
ctest -T memcheck --verbose
cd $cwd

#run topology tests
cd ./tests/topology
PYTHONUNBUFFERED=1 pytest -v -s ../test.py
