#/bin/bash

set -e

#run unit tests

ctest -V --test-dir build_debug/src/tests
ctest -V --test-dir build_debug/src/tests -T memcheck

#run topology tests

test_dir=$(pwd)/tests
cd $test_dir/topologies/basic_test/self_basic_test
python3 $test_dir/step_test.py $test_dir/step_test_cfgs/test-cfg.json


cd $test_dir/topologies/basic_test/frr_basic_test
python3 $test_dir/step_test.py $test_dir/step_test_cfgs/test-cfg.json

