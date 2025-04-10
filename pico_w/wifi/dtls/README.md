# Setup

These examples demonstrate how to use dtls via mbedtls on a Pico W device.
You need to define DTLS_SERVER and run the `makecerts.sh` script to generate the certificates and keys needed for the server and client.
```
export DTLS_SERVER=myserver
cd dtls/certs
./makecerts.sh
```
The examples should now build.

# Running the dtls examples

The client connects to a server and sends it a few lines of text which it expects to be sent back.
You can build and run the client and server examples on two Pico W devices, or to test with just one Pico W device, you can run the server or client on a Linux host.

## Using openssl

The `host/server.sh` and `host/client.sh` scripts demonstrate how to use DTLS with openssl, although you will have to echo text manually.
For example, run dtls_echo_client on a Pico W device and the `server.sh` on a linux host.
```
export DTLS_SERVER=myserver
cd host
./server.sh
```
The scripts use the keys in certs/myserver

Or run dtls_echo_server on a Pico W device and `client.sh` on a linux host. The host name for the server on Pico W is set to `pico_dtls_example`. Make sure you build the code for the Pico W and run the client with the right DTLS_SERVER name (and matching keys in the client and server) or else the SSL handshake will fail.
```
export DTLS_SERVER=pico_dtls_example
ping pico_dtls_example # make sure you can reach it!
cd host
./client.sh
```
The scripts use the keys in certs/pico_dtls_example. Type a sentence into the `client.sh` console and the server should send it back as a reply.

## Using mbedtls

The host folder contains C versions of the examples that can be compiled natively for the host. They are modified versions of mbedtls examples.
If you are building the server or client on a linux host, the mbedtls library in PICO_SDK_PATH will be used to build the code.

For example, run dtls_echo_client on a Pico W device and the dtls_host_echo_server on a linux host.
```
export DTLS_SERVER=myserver
cd host
mkdir build
cd build
cmake ..
make -j8
./dtls_host_echo_server

```
Or run dtls_echo_server on a Pico W device and dtls_host_echo_client on a linux host.
```
export DTLS_SERVER=pico_dtls_example
cd host
mkdir build
cd build
cmake ..
make -j8
./dtls_host_echo_client
```
Remember to build the client and server for the linux host and Pico W with the correct value of DTLS_SERVER or else the handshake will fail.