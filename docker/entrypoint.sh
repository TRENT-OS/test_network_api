#!/bin/bash

#-------------------------------------------------------------------------------
#
# Copyright (C) 2021, HENSOLDT Cyber GmbH
#
#-------------------------------------------------------------------------------

# Entrypoint script for the test container to be used for the demo_tls_server

set -euxo pipefail

#fix runtime uid/gid
eval $( fixuid -q )

BRIDGE_NAME=br0
TAP_INTERFACES=(tap0 tap1)
IP_ADDRESS=10.0.0.1
# netmask length in bits
NETWORK_SIZE=24

# create the bridge
sudo ip link add ${BRIDGE_NAME} type bridge

# add TAP devices to bridge
for TAP in "${TAP_INTERFACES[@]}"; do
    sudo ip tuntap add ${TAP} mode tap
    sudo ip link set ${TAP} master ${BRIDGE_NAME}
    sudo ip link set ${TAP} up
done

sudo ip addr add ${IP_ADDRESS}/${NETWORK_SIZE} dev ${BRIDGE_NAME}
sudo ip link set ${BRIDGE_NAME} up

# enable NAT of the internal network
sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
sudo iptables -A FORWARD -i ${BRIDGE_NAME} -j ACCEPT

# forward external packets through NAT
sudo iptables -t nat -A PREROUTING -i eth0 -p tcp --dport 5555:5555 -j DNAT  --to 10.0.0.11:5555
sudo iptables -t nat -A PREROUTING -i eth0 -p tcp --dport 11000:11004 -j DNAT  --to 10.0.0.11:11000-11004
sudo iptables -t nat -A PREROUTING -i eth0 -p udp --dport 11000:11004 -j DNAT  --to 10.0.0.11:11000-11004
sudo iptables -t nat -A PREROUTING -i eth0 -p udp --dport 12000:12004 -j DNAT  --to 10.0.0.11:12000-12004

# execute the command that was passed to docker run
exec "$@"
