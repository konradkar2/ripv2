#/bin/bash

set -e


cd ./tests/topology
PYTHONUNBUFFERED=1 pytest -v -s ../test.py
