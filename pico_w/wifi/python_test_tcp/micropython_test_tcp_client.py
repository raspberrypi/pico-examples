import network
import utime as time
import usocket as socket

# Set your wifi ssid and password here
WIFI_SSID = const('')
WIFI_PASSWORD = const('')

# Set the server address here like 1.2.3.4
SERVER_ADDR = const('')

# These constants should match the server
BUF_SIZE = const(2048)
SERVER_PORT = const(4242)
TEST_ITERATIONS = const(10)

# Check if wifi details have been set
if len(WIFI_SSID) == 0 or len(WIFI_PASSWORD) == 0:
    raise RuntimeError('set wifi ssid and password in this script')

# Check server ip address set
if len(SERVER_ADDR) == 0:
    raise RuntimeError('set the IP address of the server')

# Start connection
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect(WIFI_SSID, WIFI_PASSWORD)

# Wait for connect success or failure
max_wait = 20
while max_wait > 0:
    if wlan.status() < 0 or wlan.status() >= 3:
        break
    max_wait -= 1
    print('waiting for connection...')
    time.sleep(1)

# Handle connection error
if wlan.status() != 3:
    raise RuntimeError('wifi connection failed %d' % wlan.status())
else:
    print('connected')
    status = wlan.ifconfig()
    print( 'ip = ' + status[0] )

# Open socket to the server
sock = socket.socket()
addr = (SERVER_ADDR, SERVER_PORT)
sock.connect(addr)

# repeat test for a number of iterations
for test_iteration in range(TEST_ITERATIONS):
    
    # Read BUF_SIZE bytes from the server
    read_buf = sock.read(BUF_SIZE)
    print('read %d bytes from server' % len(read_buf))

    # Check size of data received
    if len(read_buf) != BUF_SIZE:
        raise RuntimeError('wrong amount of data read')

    # Send the data back to the server
    write_len = sock.write(read_buf)
    print('written %d bytes to server' % write_len)
    if write_len != BUF_SIZE:
        raise RuntimeError('wrong amount of data written')

# All done
sock.close()
print("test completed")
