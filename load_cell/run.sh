#!/bin/bash

# loop forever
while true
do
    python read_messages.py http://localhost:3000 0 IDA100 | sudo ./main | python write_records.py http://localhost:3000 0 IDA100
    echo "Restarting..."
done
