#!/bin/bash -u

#-------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
# Send TCP data via python
#-------------------------------------------------------------------------------


SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"
TEST_IP=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' `docker container ps | grep trentos_test | cut -d " " -f1`)

cd ${SCRIPT_DIR}

./send_UDP_data.py --addr $TEST_IP --port 12000 --input ../test_data/data_240kB  --output ../test_data/out_py_0  &
./send_UDP_data.py --addr $TEST_IP --port 12001 --input ../test_data/data_240kB  --output ../test_data/out_py_1  &
./send_UDP_data.py --addr $TEST_IP --port 12002 --input ../test_data/data_240kB  --output ../test_data/out_py_2  &
./send_UDP_data.py --addr $TEST_IP --port 12003 --input ../test_data/data_240kB  --output ../test_data/out_py_3  &
./send_UDP_data.py --addr $TEST_IP --port 12004 --input ../test_data/data_240kB  --output ../test_data/out_py_4  &
./send_UDP_data.py --addr $TEST_IP --port 12000 --input ../test_data/data_240kB  --output ../test_data/out_py_5  &
./send_UDP_data.py --addr $TEST_IP --port 12002 --input ../test_data/data_240kB  --output ../test_data/out_py_6  &
./send_UDP_data.py --addr $TEST_IP --port 12002 --input ../test_data/data_240kB  --output ../test_data/out_py_7  &
./send_UDP_data.py --addr $TEST_IP --port 12004 --input ../test_data/data_240kB  --output ../test_data/out_py_8  &
./send_UDP_data.py --addr $TEST_IP --port 12004 --input ../test_data/data_240kB  --output ../test_data/out_py_9  

wait 
echo "All done"