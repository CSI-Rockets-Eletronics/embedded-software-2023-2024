#!/bin/bash

# loop forever
while true
do
    python read_messages.py http://192.168.0.235:3000 0 IDA100 | sudo ./main | python write_records.py http://192.168.0.235:3000 0 IDA100
    echo "Restarting..."
done
