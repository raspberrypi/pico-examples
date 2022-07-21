import socket

# Set the server address here like 1.2.3.4
SERVER_ADDR = ''

# These constants should match the server
BUF_SIZE = 2048
SERVER_PORT = 4242
TEST_ITERATIONS = 10

# Check server ip address set
if len(SERVER_ADDR) == 0:
    raise RuntimeError('set the IP address of the server')

# Open socket to the server
sock = socket.socket()
addr = (SERVER_ADDR, SERVER_PORT)
sock.connect(addr)

# Repeat test for a number of iterations
for test_iteration in range(TEST_ITERATIONS):

    # Read BUF_SIZE bytes from the server
    read_buf = sock.recv(BUF_SIZE)
    print('read %d bytes from server' % len(read_buf))

    # Check size of data received
    if len(read_buf) != BUF_SIZE:
        raise RuntimeError('wrong amount of data read')

    # Send the data back to the server
    write_len = sock.send(read_buf)
    print('written %d bytes to server' % write_len)
    if write_len != BUF_SIZE:
        raise RuntimeError('wrong amount of data written')

# All done
sock.close()
print("test completed")
