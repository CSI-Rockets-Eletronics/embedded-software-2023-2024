#!/bin/bash -i

for port in /dev/ttyUSB{0..4}; do
    echo "Port: $port"
    address=$(python -m esptool --port "$port" read_mac | grep MAC | uniq | cut -d ' ' -f 2)

    echo "MAC: $address"

    case $output in
        "68:b6:b3:3f:07:74")
            echo "Radio rocket"
            ;;
        "68:b6:b3:3f:07:5c")
            echo "Radio ground"
            ;;
    esac
done
