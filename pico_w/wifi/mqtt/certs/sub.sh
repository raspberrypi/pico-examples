#/usr/bin/bash

if [ -z "$MQTT_SERVER" ]; then
    echo Define MQTT_SERVER
    exit 1
fi

mosquitto_sub -h $MQTT_SERVER --cafile $MQTT_SERVER/ca.crt --key $MQTT_SERVER/client.key --cert $MQTT_SERVER/client.crt -t "$1"

