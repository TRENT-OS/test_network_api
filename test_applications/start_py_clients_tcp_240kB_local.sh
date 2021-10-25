#!/bin/bash -u

#-------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
# Send TCP data via python
#-------------------------------------------------------------------------------


SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"
TEST_IP=10.0.0.11

cd ${SCRIPT_DIR}

./send_TCP_data.py --addr $TEST_IP --port 11000 --input ../test_data/data_240kB  --output ../test_data/out_py_0  &
./send_TCP_data.py --addr $TEST_IP --port 11001 --input ../test_data/data_240kB  --output ../test_data/out_py_1  &
./send_TCP_data.py --addr $TEST_IP --port 11002 --input ../test_data/data_240kB  --output ../test_data/out_py_2  &
./send_TCP_data.py --addr $TEST_IP --port 11003 --input ../test_data/data_240kB  --output ../test_data/out_py_3  &
./send_TCP_data.py --addr $TEST_IP --port 11004 --input ../test_data/data_240kB  --output ../test_data/out_py_4  &
./send_TCP_data.py --addr $TEST_IP --port 11000 --input ../test_data/data_240kB  --output ../test_data/out_py_5  &
./send_TCP_data.py --addr $TEST_IP --port 11002 --input ../test_data/data_240kB  --output ../test_data/out_py_6  &
./send_TCP_data.py --addr $TEST_IP --port 11002 --input ../test_data/data_240kB  --output ../test_data/out_py_7  &
./send_TCP_data.py --addr $TEST_IP --port 11004 --input ../test_data/data_240kB  --output ../test_data/out_py_8  &
./send_TCP_data.py --addr $TEST_IP --port 11004 --input ../test_data/data_240kB  --output ../test_data/out_py_9  

wait 
echo "All done"