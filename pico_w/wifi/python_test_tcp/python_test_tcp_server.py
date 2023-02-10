#!/usr/bin/python

import random
import socket

# Set server adress to machines IP
SERVER_ADDR = "0.0.0.0"

# These constants should match the client
BUF_SIZE = 2048
TEST_ITERATIONS = 10
SERVER_PORT = 4242

# Open socket to the server
sock = socket.socket()
sock.bind((SERVER_ADDR, SERVER_PORT))
sock.listen(1)
print("server listening on", SERVER_ADDR, SERVER_PORT)

# Wait for the client
con = None
con, addr = sock.accept()
print("client connected from", addr)

# Repeat test for a number of iterations
for test_iteration in range(TEST_ITERATIONS):
    # Generate a buffer of random data
    random_list = []
    for n in range(BUF_SIZE):
        random_list.append(random.randint(0, 255))
    write_buf = bytearray(random_list)

    # Write BUF_SIZE bytes to the client
    write_len = con.send(bytearray(write_buf))
    print('Written %d bytes to client' % write_len)

    # Check size of data written
    if write_len != BUF_SIZE:
        raise RuntimeError('wrong amount of data written %d' % write_len)

    # Read the data back from the client
    total_size = BUF_SIZE
    read_buf = b''
    while total_size > 0:
        buf = con.recv(BUF_SIZE)
        print('read %d bytes from client' % len(buf))
        total_size -= len(buf)
        read_buf += buf

    # Check size of data received
    if len(read_buf) != BUF_SIZE:
        raise RuntimeError('wrong amount of data read')

    # Check the data sent and received
    if read_buf != write_buf:
        raise RuntimeError('buffer mismatch')

# All done
con.close()
sock.close()
print("test completed")
