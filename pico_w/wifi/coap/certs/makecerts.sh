#!/usr/bin/bash

if [ "${PWD##*/}" != "certs" ]; then
    echo Run this in the certs folder
    exit 1
fi
if [ -z "$COAP_SERVER" ]; then
    echo Define COAP_SERVER
    exit 1
fi
SERVER_NAME=$COAP_SERVER

if [ -d "$SERVER_NAME" ]; then
    echo Run \"rm -fr $SERVER_NAME\" to regenerate these keys
    exit 1
fi
mkdir $SERVER_NAME
echo Generating keys in $PWD/$SERVER_NAME

openssl ecparam -name prime256v1 -genkey -noout -out $SERVER_NAME/client.key
openssl ec -in $SERVER_NAME/client.key -pubout -out $SERVER_NAME/client.pub

echo -n \#define COAP_KEY \" > $SERVER_NAME/coap_client.inc
cat $SERVER_NAME/client.key | awk '{printf "%s\\n\\\n", $0}' >> $SERVER_NAME/coap_client.inc
echo "\"" >> $SERVER_NAME/coap_client.inc
echo >> $SERVER_NAME/coap_client.inc

echo -n \#define COAP_PUB_KEY \" >> $SERVER_NAME/coap_client.inc
cat $SERVER_NAME/client.pub | awk '{printf "%s\\n\\\n", $0}' >> $SERVER_NAME/coap_client.inc
echo "\"" >> $SERVER_NAME/coap_client.inc

openssl ecparam -name prime256v1 -genkey -noout -out $SERVER_NAME/server.key
openssl ec -in $SERVER_NAME/server.key -pubout -out $SERVER_NAME/server.pub

echo -n \#define COAP_KEY \" > $SERVER_NAME/coap_server.inc
cat $SERVER_NAME/server.key | awk '{printf "%s\\n\\\n", $0}' >> $SERVER_NAME/coap_server.inc
echo "\"" >> $SERVER_NAME/coap_server.inc
echo >> $SERVER_NAME/coap_server.inc

echo -n \#define COAP_PUB_KEY \" >> $SERVER_NAME/coap_server.inc
cat $SERVER_NAME/server.pub | awk '{printf "%s\\n\\\n", $0}' >> $SERVER_NAME/coap_server.inc
echo "\"" >> $SERVER_NAME/coap_server.inc
