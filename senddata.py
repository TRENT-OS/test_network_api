#!/usr/bin/env python3

# ------------------------------------------------------------------------------
# Copyright (C) 2021, HENSOLDT Cyber GmbH
# ------------------------------------------------------------------------------

'''
Read in a data set from a file and send the messages to the destination address.
'''

import argparse
import hashlib
import socket
import sys
import time

# Default docker bridge network gateway IP
RECEIVER_ADDRESS = "172.17.0.1"
RECEIVER_PORT = 11000



# ------------------------------------------------------------------------------
def send_file(receiver_addr, filename_in, multi):
    """Establish a connection to the specified IP address and send the data to
    it.

    Args:
        outgoing_data (bytes): Bytes to be sent.
        receiver_addr (tuple): Tuple containing the destination IP address and
        port.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        #print("open f_out: ", filename_out)
        #print ("sock_connect: ", filename_out)
        sock.connect(receiver_addr)
        #print ("sock_connect successfull: ", filename_out)
        with open(filename_in, "rb") as f_in:
            print("read data")
            # read the bytes from the file
            bytes_read = f_in.read()
            # we use sendall to assure transimission in
            #print(bytes_read)
            total_bytes_read = b""
            for i in range (0, multi):
                total_bytes_read += bytes_read
            print(f"Sending {len(total_bytes_read)} BYTES")
            # busy networks
            sock.sendall(total_bytes_read)
            # receive data back

    except ConnectionError:
        print("Could not connect to {}:{}.".format(*receiver_addr))
        print("Check if the receiving application is running.")
        raise

    finally:
        sock.close()
        f_in.close()
        print("socket closed")


# ------------------------------------------------------------------------------
def main() -> int:
    parser = argparse.ArgumentParser(
        description="""Read in a data set from a file and send the messages to
        the destination address.""")
    parser.add_argument('--input', required=True, help='file to read the data set from')
    parser.add_argument('--addr', required=False, default=RECEIVER_ADDRESS, help='destination IP address')
    parser.add_argument('--port', required=False, default=RECEIVER_PORT, type=int, help='destination port')
    parser.add_argument('--multi', required=False, default=1, type=int, help='multiply data')
    parser.add_argument('--runs', required=False, default=1, type=int, help='run test multiple times')
    args = parser.parse_args()

    try:

        # Send all the collected messages to the configured receiver.
        receiver_addr = (args.addr, args.port)
        print("Sending all messages to {}".format(*receiver_addr))

        for i in range(0, args.runs):
            print(f"Run {i+1} of {args.runs}")
            send_file(receiver_addr, args.input, args.multi)

        print("All messages transmitted")

    except KeyboardInterrupt:
        print('Aborted manually.', file=sys.stderr)
        return 1

    except Exception as err:
        print(err)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
