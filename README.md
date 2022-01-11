# test\_network\_api

Test the OS network API

## Getting Started

The project is to be included in a TRENTOS layout with seos\_sanbox (like
seos\_tests) to build a binary image.

### Dependencies

* seos\_sandbox

## Test configurations

This test can build for multiple configurations of the test environment. The
following single-NIC targets are currently supported:

* tcp_client_single_socket
* tcp_client_multiple_sockets
* tcp_client_multiple_clients
* tcp_server
* udp_server

To build test_network_api in a given configuration

```bash
BUILD_PLATFORM=zynq7000 src/build.sh test_network_api \
-DTEST_CONFIGURATION=<test_config_name>
```

To run the corresponding tests:

```bash
BUILD_PLATFORM=zynq7000 src/build.sh test-run test_network_api.py \
--tc=platform.test_configuration:<test_config_name>
```

## Running tests on hardware

In the CMakeLists.txt set the IP addresses for the network stacks using the
macros:

```bash
-DDEV_ADDR="192.168.82.231"
-DGATEWAY_ADDR="192.168.82.1"
-DSUBNET_MASK="255.255.255.0"
```

DEV_ADDR will be the address of your board, GATEWAY_ADDR will be the address of
the PC running the test container. SUBNET_MASK depends on the network, but most
likely won't be different from the default value shown here.

Start the network test in the trentos_test container using the following
command:

```bash
src/build.sh test-run test_network_api.py --tc=platform.uart_connected:false
--tc=network.client_ip:192.168.82.231 --tc=network.server_ip:192.168.82.232
```

Note that not all test will function in this setup (if they require the board
and the test container to be in the same network or base their decision only on
UART output).
