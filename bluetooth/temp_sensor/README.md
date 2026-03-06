### Temp sensor

This example uses BLE to communicate temperature between a pair of pico Ws. It reads temperature by measuring some onboard voltage and applying a conversion factor.

temp_server is a peripheral or server that transmits its temperature to another device
temp_reader is a client that reads a temperature from another device