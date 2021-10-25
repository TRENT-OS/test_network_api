#!/bin/bash -u

#-------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
# Send UDP data via netcat
#-------------------------------------------------------------------------------


SCRIPT_DIR="$(cd "$(dirname "$0")" >/dev/null 2>&1 && pwd)"
TEST_IP=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' `docker container ps | grep trentos_test | cut -d " " -f1`)

cd ${SCRIPT_DIR}

netcat -v -u -w 5 $TEST_IP 12000 <../test_data/data_240kB >../test_data/out_0  &
netcat -v -u -w 5 $TEST_IP 12001 <../test_data/data_240kB >../test_data/out_1  &
netcat -v -u -w 5 $TEST_IP 12002 <../test_data/data_240kB >../test_data/out_2  &
netcat -v -u -w 5 $TEST_IP 12002 <../test_data/data_240kB >../test_data/out_3  &
netcat -v -u -w 5 $TEST_IP 12003 <../test_data/data_240kB >../test_data/out_4  &
netcat -v -u -w 5 $TEST_IP 12003 <../test_data/data_240kB >../test_data/out_5  &
netcat -v -u -w 5 $TEST_IP 12004 <../test_data/data_240kB >../test_data/out_6  &
netcat -v -u -w 5 $TEST_IP 12004 <../test_data/data_240kB >../test_data/out_7  &
netcat -v -u -w 5 $TEST_IP 12004 <../test_data/data_240kB >../test_data/out_8

wait 
echo "All done"