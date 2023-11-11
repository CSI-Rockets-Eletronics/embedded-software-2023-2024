#!/bin/bash

set -eo pipefail

# Get the usb device to unbind
output=$(ls /sys/bus/usb/drivers/ftdi_sio)
# Split the output into an array
output_array=($output)
# Get the first element of the array
usb_device=${output_array[0]}

# if $usb_device is "bind", then the device is already unbound
if [ "$usb_device" != "bind" ]; then
    # Unbind the usb device
    echo -n $usb_device | sudo tee /sys/bus/usb/drivers/ftdi_sio/unbind
fi
