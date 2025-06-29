#!/usr/bin/bash

SERVER_ADDR=${DTLS_SERVER:-$1}
if [ -z "$SERVER_ADDR" ]; then
    echo Pass dtls server address as a parameter or set DTLS_SERVER
    exit 1
fi
CERT_FOLDER=../certs/$SERVER_ADDR
if [ ! -e $CERT_FOLDER/server.crt ]; then
	echo Cannot find server certificate
	exit 2
fi
echo Waiting for client to connect. Manually echo text received
openssl s_server -dtls -cert $CERT_FOLDER/server.crt -key $CERT_FOLDER/server.key -verifyCAfile $CERT_FOLDER/ca.crt -timeout
