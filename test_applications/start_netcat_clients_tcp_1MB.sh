#!/bin/bash -u

#-------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
# Send TCP data via netcat
#-------------------------------------------------------------------------------


SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"
TEST_IP=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' `docker container ps | grep trentos_test | cut -d " " -f1`)

cd ${SCRIPT_DIR}

netcat -v -w 5 $TEST_IP 11000 <../test_data/data_1MB >../test_data/out_0  &
netcat -v -w 5 $TEST_IP 11001 <../test_data/data_1MB >../test_data/out_1  &
netcat -v -w 5 $TEST_IP 11002 <../test_data/data_1MB >../test_data/out_2  &
netcat -v -w 5 $TEST_IP 11002 <../test_data/data_1MB >../test_data/out_3  &
netcat -v -w 5 $TEST_IP 11003 <../test_data/data_1MB >../test_data/out_4  &
netcat -v -w 5 $TEST_IP 11003 <../test_data/data_1MB >../test_data/out_5  &
netcat -v -w 5 $TEST_IP 11004 <../test_data/data_1MB >../test_data/out_6  &
netcat -v -w 5 $TEST_IP 11004 <../test_data/data_1MB >../test_data/out_7  &
netcat -v -w 5 $TEST_IP 11004 <../test_data/data_1MB >../test_data/out_8

wait 
echo "All done"