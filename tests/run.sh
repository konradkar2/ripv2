#/bin/bash

set -e

#run unit tests

ctest -V --test-dir build_debug/src/tests
ctest -V --test-dir build_debug/src/tests -T memcheck

#run topology tests
test_root_dir=$(pwd)
cd ./tests/frr_test
pytest -v -s ../basic_test.py --name frr_test
cd $test_root_dir
cd ./tests/self_test
pytest -v -s ../basic_test.py --name self_test

