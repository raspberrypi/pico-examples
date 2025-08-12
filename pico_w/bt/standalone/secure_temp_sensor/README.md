### Secure temp sensor

This example uses BLE to communicate temperature between a pair of pico Ws. This example is a variant of temp sensor, using LE secure to provide a secure connection. 

In server.c and client.c there is a variable security_setting which you can change to explore different security options:

Security setting 0: Just works (pairing), no MITM protection
Security setting 1: Numeric comparison
Security setting 2: Peripheral displays passkey, client enters passkey
Security setting 3: Client displays passkey, peripheral enters passkey 