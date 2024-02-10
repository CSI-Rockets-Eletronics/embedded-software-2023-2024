#!/bin/bash -i

SETUP_FILES_DIR=$(realpath "$(dirname "$0")")

# Copy the service file to the systemd services folder
cd "$SETUP_FILES_DIR"
sudo cp load-cell.service /etc/systemd/system/

# Reload systemd daemon to read the new service file
sudo systemctl daemon-reload

# Enable the service to run on boot
sudo systemctl enable load-cell.service

# Start the service
sudo systemctl restart load-cell.service

# Disable the ftdi_sio and usbserial kernel modules (using fake install)
sudo rm -f /etc/modprobe.d/blacklist-ftdi_sio.conf
sudo rm -f /etc/modprobe.d/blacklist-usbserial.conf
sudo echo "install ftdi_sio /bin/true" | sudo tee /etc/modprobe.d/blacklist-ftdi_sio.conf
sudo echo "install usbserial /bin/true" | sudo tee /etc/modprobe.d/blacklist-usbserial.conf
echo "Please reboot the system to apply the changes"
 