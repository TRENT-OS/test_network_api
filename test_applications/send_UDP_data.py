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
RECEIVER_PORT = 12000



# ------------------------------------------------------------------------------
def send_file(receiver_addr, filename_in, filename_out):
    """Establish a connection to the specified IP address and send the data to
    it.

    Args:
        outgoing_data (bytes): Bytes to be sent.
        receiver_addr (tuple): Tuple containing the destination IP address and
        port.
    """
    bufferSize          = 1024
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        #print("open f_out: ", filename_out)
        f_out = open(filename_out, "wb+")
       
        with open(filename_in, "rb") as f_in:
            while True:
                #print("f_in.read()")
                # read the bytes from the file
                bytes_read = f_in.read(bufferSize)
                if not bytes_read:
                    # file transmitting is done
                    break
                # we use sendall to assure transimission in 
                # busy networks
                #print("sock.sendto()", filename_out)
                sock.sendto(bytes_read, receiver_addr)

                # receive data back
                #print("sock.recvfrom()", filename_out)
                bytes_received = sock.recvfrom(len(bytes_read))
                #print("f_out.write()", filename_out)
                f_out.write(bytes_received[0])

    except ConnectionError:
        print("Could not connect to {}:{}.".format(*receiver_addr))
        print("Check if the receiving application is running.")
        raise

    finally:
        sock.close()
        f_in.close()
        f_out.close()
        print("stop loop:  ", receiver_addr, filename_out)


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

        send_file(receiver_addr, args.input, args.output)
        print("All messages successfully transmitted - ", args.output)

    except KeyboardInterrupt:
        print('Aborted manually.', file=sys.stderr)
        return 1

    except Exception as err:
        print(err)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
