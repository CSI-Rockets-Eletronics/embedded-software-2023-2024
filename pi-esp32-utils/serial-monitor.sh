#!/bin/bash -i

# prompt for the port
read -p "Port num (e.g. \"1\" for /dev/ttyUSB1): " port

python3 -m serial.tools.miniterm "/dev/ttyUSB$port" 115200
