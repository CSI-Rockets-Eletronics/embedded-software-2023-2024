$SSH_DOMAIN = "9.tcp.ngrok.io"
$SSH_PORT = 23364

# List all .bin files in the ./build directory
$files = Get-ChildItem -Path "./build" -Filter "*.bin" -File

# Check if there are no files
if ($files.Count -eq 0) {
    Write-Host "No files found!"
    exit 1
}

# Remove the old build directory and create a new one
ssh -p $SSH_PORT "victator@$SSH_DOMAIN" "rm -rf ~/esp32-build-temp && mkdir ~/esp32-build-temp"

# Send files over scp
foreach ($file in $files) {
    scp -P $SSH_PORT "$($file.FullName)" "victator@$SSH_DOMAIN:~/esp32-build-temp/"
}
