#!/bin/bash -i

for port in /dev/ttyUSB{0..4}; do
    echo "Port $port:"
    address=$(python -m esptool --port "$port" read_mac | grep MAC | uniq | cut -d ' ' -f 2)

    if [ -z "$address" ]; then
        continue
    fi

    echo -e "MAC: $address\t\c"

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
        "68:b6:b3:3f:33:f8")
            echo "Scientific"
            ;;
        *)
            echo "Unknown"
            ;;
    esac
done
