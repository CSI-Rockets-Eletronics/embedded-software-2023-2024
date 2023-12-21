#!/bin/bash -i

sudo apt update
sudo apt install -y python3-pip
pip3 install pyserial --break-system-packages
pip3 install esptool --break-system-packages
