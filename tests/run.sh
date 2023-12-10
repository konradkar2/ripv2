#/bin/bash

set -e

#run unit tests
ctest --test-dir build_debug/src/tests -T memcheck

#run topology tests
cd ./tests/topology
PYTHONUNBUFFERED=1 pytest -v -s ../test.py
