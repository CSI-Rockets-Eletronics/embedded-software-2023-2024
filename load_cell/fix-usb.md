Windows:

```
usbipd list
usbipd wsl attach --busid 1-1
```

WSL

```
lsusb -t
ls /sys/bus/usb/drivers/ftdi_sio
echo -n 1-1:1.0 | sudo tee /sys/bus/usb/drivers/ftdi_sio/unbind
```

Or, together on Windows:

```
usbipd list
usbipd wsl attach --busid 1-1 && wsl -u root -e bash -c "sleep 1 && (echo -n 1-1:1.0 | tee /sys/bus/usb/drivers/ftdi_sio/unbind)"
```
