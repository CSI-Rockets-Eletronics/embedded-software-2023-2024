#!/bin/bash -i

REPO_DIR=$(realpath "$(dirname "$0")/..")

# Go to the repo directory
cd $REPO_DIR

# Create a python venv and install pyserial
sudo apt update
sudo apt install -y python3-venv
python -m venv env
source env/bin/activate
pip install pyserial
