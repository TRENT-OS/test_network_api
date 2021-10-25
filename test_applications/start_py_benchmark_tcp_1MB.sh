#!/bin/bash -u

#-------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
# Send TCP data via python
#-------------------------------------------------------------------------------


SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"
TEST_IP=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' `docker container ps | grep trentos_test | cut -d " " -f1`)

cd ${SCRIPT_DIR}

./send_TCP_data.py --addr $TEST_IP --port 11000 --input ../test_data/data_1MB  --output ../test_data/out_py_benchmark

wait 
echo "All done"