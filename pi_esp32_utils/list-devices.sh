#!/bin/bash -i

for port in /dev/ttyUSB{0..3}; do
    echo "Port $port:"
    address=$(python -m esptool --chip esp32s3 --port "$port" read_mac | grep MAC | uniq | cut -d ' ' -f 2)

    # avoid spamming the serial port
    sleep 1s

    if [ -z "$address" ]; then
        continue
    fi

    echo -e "$address\t\c"

    case $address in
        "68:b6:b3:3f:07:74")
            echo "Radio rocket"
            ;;
        "68:b6:b3:3f:07:5c")
            echo "Radio ground"
            ;;
        "68:b6:b3:3e:87:a0")
            echo "GPS"
            ;;
        "68:b6:b3:3d:75:f8")
            echo "Scientific"
            ;;
        "34:85:18:a1:fa:10")
            echo "Flight computer"
            ;;
        *)
            echo "Unknown"
            ;;
    esac
done
