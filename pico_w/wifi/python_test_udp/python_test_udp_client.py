import socket
import time

# === Configuration ===
PICO_IP = "PI_IP_HERE"  # Replace with your Pico's IP
PICO_PORT = 12345       # Port Pico is listening on

# === Data to send ===
data = {
    "1.": "Message 1 received!",
    "2.": "Message 2 received!",
    "3.": "Message 3 received!",
    "4.": "Message 4 received!",
    "5.": "Message 5 received!"
}

# === Create UDP socket ===
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try:
    for key, value in data.items():
        message = f"{key}:{value}"
        print(f"Sending to {PICO_IP}:{PICO_PORT} -> {message}")
        sock.sendto(message.encode(), (PICO_IP, PICO_PORT))
        time.sleep(0.1)  # slight delay to prevent message loss

except Exception as e:
    print(f"Error: {e}")

finally:
    sock.close()
