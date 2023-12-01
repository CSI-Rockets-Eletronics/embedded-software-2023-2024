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
