#/bin/bash

set -e

#run unit tests

ctest -V --test-dir build_debug/src/tests
ctest -V --test-dir build_debug/src/tests -T memcheck

#run topology tests
cd ./tests/topology
PYTHONUNBUFFERED=1 pytest -v -s ../test_basic.py
