#/bin/bash

set -e


cd ./tests/topology
pytest -v -s  ../test.py
