#!/bin/bash

# function to kill all background processes
kill_all() {
    pkill -P $$
    exit 0
}

# Trap the SIGINT signal (Ctrl+C) and call the kill_all function
trap kill_all SIGINT

./unbind.sh

# remove and make pipes
rm -f  read1 read2 write1 write2
mkfifo read1 read2 write1 write2

# start read_messages.py in background
python read_messages.py http://localhost:3000 0 LoadCell1 > read1 &
python read_messages.py http://localhost:3000 0 LoadCell2 > read2 &

# start write_messages.py in background
python write_records.py http://localhost:3000 0 LoadCell1 < write1 &
python write_records.py http://localhost:3000 0 LoadCell2 < write2 &

# start main in background
./main 652964 1750 < read1 > write1 &
./main 1076702 1750 < read2 > write2 &

# wait for any of the background processes to finish
wait -n

# kill the other background processes
kill_all

# wait for the background processes to finish
wait
