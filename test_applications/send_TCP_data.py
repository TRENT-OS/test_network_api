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
def send_file(receiver_addr, filename_in, filename_out):
    """Establish a connection to the specified IP address and send the data to
    it.

    Args:
        outgoing_data (bytes): Bytes to be sent.
        receiver_addr (tuple): Tuple containing the destination IP address and
        port.
    """
    bufferSize = 1024
    byte_count = 0
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        #print("open f_out: ", filename_out)
        f_out = open(filename_out, "wb+")
        #print ("sock_connect: ", filename_out)
        sock.connect(receiver_addr)
        #print ("sock_connect successfull: ", filename_out)
        with open(filename_in, "rb") as f_in:
            while True:
                #print("read data 1")
                # read the bytes from the file
                bytes_read = f_in.read(bufferSize)
                if not bytes_read:
                    # file transmitting is done
                    break
                # we use sendall to assure transimission in 
                # busy networks
                sock.sendall(bytes_read)
                # receive data back
                bytes_received = sock.recv(len(bytes_read))
                #print("read data 2: bytes received: ", len(bytes_read))
                f_out.write(bytes_received)
                byte_count = byte_count + len(bytes_read)


    except ConnectionError:
        print("Could not connect to {}:{}.".format(*receiver_addr))
        print("Check if the receiving application is running.")
        raise

    finally:
        sock.close()
        f_in.close()
        f_out.close()
        #print("stop loop:  ", receiver_addr, filename_out)
        return byte_count


# ------------------------------------------------------------------------------
def main() -> int:
    parser = argparse.ArgumentParser(
        description="""Read in a data set from a file and send the messages to
        the destination address.""")
    parser.add_argument('--input', required=True,
                        help='file to read the data set from')
    parser.add_argument('--output', required=True,
                        help='file to write the received data to')
    parser.add_argument('--addr', required=False,
                        default=RECEIVER_ADDRESS,
                        help='destination IP address')
    parser.add_argument('--port', required=False,
                        default=RECEIVER_PORT, type=int,
                        help='destination port')

    args = parser.parse_args()

    try:

        # Send all the collected messages to the configured receiver.
        receiver_addr = (args.addr, args.port)
        print("Sending all messages to {}:{}, ".format(*receiver_addr), args.output)
        start = time.time()
        send_bytes = send_file(receiver_addr, args.input, args.output)
        stop = time.time()
        print("All messages successfully transmitted - ",
              args.output,
              ", dur:",
              stop-start,
              "s, speed: ",
              send_bytes/(stop-start),
              "bytes/s")

    except KeyboardInterrupt:
        print('Aborted manually.', file=sys.stderr)
        return 1

    except Exception as err:
        print(err)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
