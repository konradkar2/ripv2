#/bin/bash

set -e

#run unit tests

ctest -V --test-dir build_debug/src/tests
ctest -V --test-dir build_debug/src/tests -T memcheck

#run topology tests
test_root_dir=$(pwd)
cd ./tests/frr_test
#pytest -v -s ../step_test.py
#python3 ../step_test.py test-cfg.json

cd $test_root_dir
cd ./tests/self_test
python3 ../step_test.py

