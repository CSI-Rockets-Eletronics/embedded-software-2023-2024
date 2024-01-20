#!/bin/bash

set -eo pipefail

# Get the usb devices to unbind
output=$(ls /sys/bus/usb/drivers/ftdi_sio)
# Split the output into an array
output_array=($output)

# Loop through the output array
for usb_device in "${output_array[@]}"; do
    # All the devices that we want to unbind are listed before the "bind" entry
    if [ "$usb_device" == "bind" ]; then
        break
    fi

    # Unbind the usb device
    echo -n $usb_device | sudo tee /sys/bus/usb/drivers/ftdi_sio/unbind
done
