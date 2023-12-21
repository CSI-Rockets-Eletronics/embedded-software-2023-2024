#!/bin/bash -i

SCRIPT_DIR=$(realpath "$(dirname "$0")")

# prompt for the port
read -p "Port num (e.g. \"1\"for /dev/ttyUSB1): " port

# find file ending in .ino.bootloader.bin
bootloader=$(find ~/esp32-build-temp -type f -name "*.ino.bootloader.bin")
if [ -z "$bootloader" ]; then
    echo "No .ino.bootloader.bin file found!"
    exit 1
fi

# find file ending in .ino.partitions.bin
partitions=$(find ~/esp32-build-temp -type f -name "*.ino.partitions.bin")
if [ -z "$partitions" ]; then
    echo "No .ino.partitions.bin file found!"
    exit 1
fi

# find file ending in .ino.bin
main=$(find ~/esp32-build-temp -type f -name "*.ino.bin")
if [ -z "$main" ]; then
    echo "No .ino.bin file found!"
    exit 1
fi

python3 -m esptool --chip esp32s3 --port "/dev/ttyUSB$port" --baud 921600 \
    --before default_reset --after hard_reset write_flash -z --flash_mode dout --flash_freq 80m --flash_size 4MB \
    0x0 $bootloader 0x8000 $partitions 0xe000 "$SCRIPT_DIR/boot_app0.bin" 0x10000 $main
