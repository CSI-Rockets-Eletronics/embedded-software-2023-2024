for port in /dev/ttyUSB{0..4}; do
    python -m esptool --port "$port" read_mac | grep MAC | uniq
done
