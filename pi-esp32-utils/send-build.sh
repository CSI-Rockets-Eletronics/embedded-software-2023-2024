#!/bin/bash -i

# List all .ino files in the ./build directory
files=$(find ./build -type f -name "*.bin")

# Check if there are no files
if [ -z "$files" ]; then
    echo "No files found!"
    exit 1
fi

# Remove the old build directory and create a new one
ssh -p 12939 victator@4.tcp.ngrok.io "rm -rf ~/esp32-build-temp && mkdir ~/esp32-build-temp"

# Send files over scp
for file in $files; do
    scp -P 12939 "$file" victator@4.tcp.ngrok.io:~/esp32-build-temp/
done
