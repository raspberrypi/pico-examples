#!/usr/bin/bash

SERVER_PORT=4433
SERVER_ADDR=${DTLS_SERVER:-$1}
if [ -z "$SERVER_ADDR" ]; then
    echo Pass dtls server address as a parameter or set DTLS_SERVER
    exit 1
fi
CERT_FOLDER=../certs/$SERVER_ADDR
if [ ! -e $CERT_FOLDER/client.crt ]; then
	echo Cannot find client certificate
	exit 2
fi
echo Connecting to $SERVER_ADDR
echo Enter some text to send. Enter \"Q\" to exit
openssl s_client -dtls -cert $CERT_FOLDER/client.crt -key $CERT_FOLDER/client.key -verifyCAfile $CERT_FOLDER/ca.crt -timeout -connect $SERVER_ADDR:${SERVER_PORT}
