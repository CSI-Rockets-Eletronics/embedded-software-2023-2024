#!/bin/bash -i

SSH_DOMAIN=9.tcp.ngrok.io
SSH_PORT=23364

# List all .ino files in the ./build directory
files=$(find ./build -type f -name "*.bin")

# Check if there are no files
if [ -z "$files" ]; then
    echo "No files found!"
    exit 1
fi

# Remove the old build directory and create a new one
ssh -p $SSH_PORT "victator@$SSH_DOMAIN" "rm -rf ~/esp32-build-temp && mkdir ~/esp32-build-temp"

# Send files over scp
for file in $files; do
    scp -P $SSH_PORT "$file" "victator@$SSH_DOMAIN:~/esp32-build-temp/"
done
