# Quick start

To use this example you will need to install an MQTT server on your network.
To install the Mosquitto MQTT client on a Raspberry Pi Linux device run the following...

```
sudo apt install mosquitto
sudo apt install mosquitto-clients
```

Check it works...

```
mosquitto_pub -t test_topic -m "Yes it works" -r
mosquitto_sub -t test_topic -C 1
```

To allow an external client to connect to the server you will have to change the configuration. Add the following to /etc/mosquitto/conf.d/mosquitto.conf

```
allow_anonymous true
listener 1883 0.0.0.0
```

Then restart the service.

```
sudo service mosquitto restart
```

When building the code set the host name of the MQTT server, e.g.

```
export MQTT_SERVER=myhost
cmake ..
```

The example should publish its core temperature to the /temperature topic. You can subscribe to this topic from another machine.

```
mosquitto_sub -h $MQTT_SERVER -t '/temperature'
```

You can turn the led on and off by publishing messages.

```
mosquitto_pub -h $MQTT_SERVER -t '/led' -m on
mosquitto_pub -h $MQTT_SERVER -t '/led' -m off
```

# Security

If your server has a username and password, you can set these with the following variables.

```
export MQTT_USERNAME=user
export MQTT_PASSWORD=pass
```

Be aware that these details are sent in plain text unless you use TLS.

## Using TLS

To use TLS and client server authentication you need some keys and certificates for the client and server.
The `certs/makecerts.sh` script demonstrates a way to make these.
It creates a folder named after `MQTT_SERVER` containing all the required files.
From these files it generates a header file `mqtt_client.inc` included by the code.
Your server will have to be configured to use TLS and the port 8883 rather than the non-TLS port 1883.

```
listener 1883 127.0.0.1

listener 8883 0.0.0.0
allow_anonymous true
cafile /etc/mosquitto/ca_certificates/ca.crt
certfile /etc/mosquitto/certs/server.crt
keyfile /etc/mosquitto/certs/server.key
require_certificate true
```

To connect to your server with the mosquitto tools in linux you will have to pass extra parameters.

```
mosquitto_pub -h $MQTT_SERVER --cafile $MQTT_SERVER/ca.crt --key $MQTT_SERVER/client.key --cert $MQTT_SERVER/client.crt -t /led -m on
mosquitto_sub -h $MQTT_SERVER --cafile $MQTT_SERVER/ca.crt --key $MQTT_SERVER/client.key --cert $MQTT_SERVER/client.crt -t "/temperature"
```

There are some shell scripts in the certs folder to reduce the amount of typing assuming `MQTT_SERVER` is defined.

```
cd certs
./pub.sh /led on
./sub.sh /temperature
```
