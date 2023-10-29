#/bin/bash

set -e

rip_log_file=/tmp/test_ns/r3/rip.log

cd ./tests/topology
python3 ../test.py &
test_pid=$!

retries=0
max_retries=30
while [ ! -f $rip_log_file ]; do
    sleep 1
    retries=$((retries + 1))
    if [ $retries == $max_retries ]; then
        echo "Timeout on creating $rip_log_file, exiting.."
        exit 1
    fi
done
tail -f $rip_log_file

wait $test_pid