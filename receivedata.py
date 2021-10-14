#!/usr/bin/env python3

# ------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
# ------------------------------------------------------------------------------

'''
Listen to incoming TCP-traffic and count the amount of data received.
'''

import argparse
import socket
import sys

# Default address and port

HOST = '127.0.0.1'  # localhost
PORT = 5555         # non-privileged ports are > 1023

# ------------------------------------------------------------------------------

def receive_file(addr, port):
    while True:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind((addr, port))
        sock.listen()
        conn, client_addr = sock.accept()
        print("socket acceptet")
        received_bytes = 0
        with conn:
            print(f"Connected by {client_addr}")
            while True:
                data = conn.recv(1024)
                received_bytes += len(data)
                if not data:
                    print(f"received {received_bytes} bytes of data")
                    break



# ------------------------------------------------------------------------------
def main() -> int:
    parser = argparse.ArgumentParser(
        description="""Read in a data set from a file and send the messages to
        the destination address.""")
    parser.add_argument('--addr', required=False,
                        default=HOST,
                        help='destination IP address')
    parser.add_argument('--port', required=False,
                        default=PORT, type=int,
                        help='destination port')

    args = parser.parse_args()

    try:
        print(f"Server listening on {args.addr}:{args.port}")
        receive_file(args.addr, args.port)

    except KeyboardInterrupt:
        print('Aborted manually.', file=sys.stderr)
        return 1

    except Exception as err:
        print(err)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
