# test_network_api Multiserver

The multiserver opens 5 TCP server sockets on the ports 11000-11004 and 5 UDP server sockets on ports 12000 - 12004.
This file explains how to use them

## Build

### test_network_api in multiserver setup

```bash
src/seos_sandbox/scripts/open_trentos_build_env.sh src/build.sh test_network_api -DTEST_CONFIGURATION=multi_server
```

### Proxy

```bash
src/seos_sandbox/scripts/open_trentos_build_env.sh src/seos_sandbox/tools/proxy/build.sh src/seos_sandbox
```

## Run

```bash
src/seos_sandbox/scripts/open_trentos_test_env.sh -d "-v $(pwd)/src/src/tests/test_network_api/docker:/docker" -d "--entrypoint=/docker/entrypoint.sh" src/src/tests/test_network_api/run_demo.sh build-zynq7000-Debug-test_network_api build_proxy
```

## Test Applications
The test_applications folder includes several scripts to initiate TCP and UDP data transfers.
Test applications are as follows:
* send_TCP_data.py - python script for sending and receiving TCP data 
* send_UDP_data.py - python script for sending and receiving UDP data 
* using netcat for transfer and sending TCP data
* netcat with UDP doesn't work in this setup. There were always issues, for which no solution was found.

To ease initiating parallel data transfers several scripts are available:
* start_py_clients_tcp_240kB.sh - python script starting 9 client TCP connections (QEMU)
* start_py_clients_tcp_1MB.sh - python script starting 9 client TCP connections (QEMU)
* start_py_clients_udp_240kB.sh - python script starting 9 client UDP connections (QEMU)
* start_py_clients_tcp_240kB_local.sh - python script starting 9 client TCP connection  (10.0.0.11)
* start_py_benchmark_tcp_1MB.sh - python script starting 1 client TCP connection (QEMU)
* start_netcat_clients_tcp_240kB.sh - script starting 9 client TCP connections with netcat (QEMU)
* start_netcat_clients_tcp_1MB.sh - script starting 9 client TCP connections with netcat (QEMU)
* start_netcat_clients_udp_240kB.sh - script starting 9 client UCP connections with netcat, not working (QEMU)

### starting test application scripts
```bash
test_applications/start_py_clients_tcp_240kB.sh
```
Note: This will create in the folder /test_data new output files.

### Get IP of QEMU test container

* Get ID of container instance (first coloumn)

```bash

docker container ps | grep trentos_test
```

* Get IP of container instance in TEST_IP

```bash
TEST_IP=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' CONTAINER_ID_FROM_PREVIOS_STEP)

# Or as a single command

TEST_IP=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' `docker container ps | grep trentos_test | cut -d " " -f1`)
```
