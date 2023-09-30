#/bin/bash

set -e

cd ./tests/topology
python3 ../test.py &
test_pid=$!
sleep 5
tail -f "/tmp/test_ns/r1/rip.log" 
wait $test_pid