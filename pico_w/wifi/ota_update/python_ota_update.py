#!/usr/bin/python

import socket
import sys
import hashlib

# Check server ip address set
if len(sys.argv) < 2:
    raise RuntimeError('pass IP address of the server')

# Check file is set
if len(sys.argv) < 3:
    raise RuntimeError('pass UF2 file for update')

# Set the server address here like 1.2.3.4
SERVER_ADDR = sys.argv[1]

# These constants should match the server
BUF_SIZE = 2048
UF2_BLOCK = 512
SERVER_PORT = 4242

# Open socket to the server
sock = socket.socket()
addr = (SERVER_ADDR, SERVER_PORT)
sock.connect(addr)

file = sys.argv[2]

with open(file, 'rb') as f:
    data = f.read()

data = bytearray(data)

# Skip abs block
data = data[UF2_BLOCK:]

print("Len data", len(data), f"/{BUF_SIZE}", len(data) / BUF_SIZE)

while (len(data) % BUF_SIZE != 0):
    data.extend([0] * UF2_BLOCK)

print("Len data now", len(data), f"/{BUF_SIZE}", len(data) / BUF_SIZE)

# Repeat test for a number of iterations
for i in range(len(data) // BUF_SIZE):
    print("I", i, "of", len(data) // BUF_SIZE, ' '*10, end='\r')
    uf2_buf = data[i*BUF_SIZE:(i+1)*BUF_SIZE]

    # Send the data back to the server
    write_len = sock.send(uf2_buf)
    if write_len != BUF_SIZE:
        raise RuntimeError('wrong amount of data written')

    if i < (len(data) // BUF_SIZE) - 1:
        # Read 32 bytes from the server
        total_size = 32
        read_buf = b''
        while total_size > 0:
            buf = sock.recv(32)
            # print('read %d bytes from server' % len(buf))
            total_size -= len(buf)
            read_buf += buf

        # Check size of data received
        if len(read_buf) != 32:
            raise RuntimeError('wrong amount of data read %d', len(read_buf))

        # Check the hash matches
        h = hashlib.new('sha256')
        h.update(uf2_buf)
        if read_buf != h.digest():
            raise RuntimeError('buffer mismatch')

# All done
sock.close()
print("\nupload completed")
