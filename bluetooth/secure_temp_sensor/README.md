### Secure temp sensor

This example uses BLE to communicate temperature between a pair of pico Ws. This example is a variant of temp sensor, using LE secure to provide a secure connection. 

secure_temp_server is a peripheral or server that transmits its temperature to another device
secure_temp_client is a client that reads a temperature from another device

In server.c and client.c there is a define SECURITY_SETTING which you can change to explore different security options:

security setting 0: Just works (pairing), no MITM (Man In The Middle) protection
 client and server have no input or output support

security setting 1: Numeric comparison with MITM protection
 client can query yes or no from the user, server has a display only
 server displays passkey
 client displays passkey and user can select Yes or No if they agree the passkey is from the server

security setting 2:
 client has a keyboard and display, server has a display only
 server displays passkey
 client user enters the passkey displayed by the server

security setting 3:
 client has a display only, server has a display and keyboard
 Client displays passkey
 server user enters the passkey displayed by the server

You will need to use the console with both devices to see the passkeys and answer security prompts. Both stdio over UART and USB are enabled so you can use either.
