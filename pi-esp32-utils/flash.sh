#!/bin/bash -i

SCRIPT_DIR=$(realpath "$(dirname "$0")")

# prompt for the port
read -p "Port num (e.g. \"1\"for /dev/ttyUSB1): " port

python3 -m esptool --chip esp32s3 --port "/dev/ttyUSB$port" --baud 921600 \
    --before default_reset --after hard_reset write_flash -z --flash_mode dout --flash_freq 80m --flash_size 4MB \
    0x0 ~/esp32-build-temp/radio_ga_ga_ground.ino.bootloader.bin \
    0x8000 ~/esp32-build-temp/radio_ga_ga_ground.ino.partitions.bin \
    0xe000 "$SCRIPT_DIR/boot_app0.bin" \
    0x10000 ~/esp32-build-temp/radio_ga_ga_ground.ino.bin 
