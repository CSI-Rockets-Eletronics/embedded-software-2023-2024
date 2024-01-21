#!/bin/bash

# function to kill all background processes
kill_all() {
    pkill -P $$
}

# Trap the SIGINT signal (Ctrl+C) and call the kill_all function
trap kill_all SIGINT

./unbind.sh

# remove pipes if they exist
if [ -p pipe0 ]; then
    rm pipe0
fi
if [ -p pipe1 ]; then
    rm pipe1
fi

# make pipes
mkfifo pipe0
mkfifo pipe1

# start read_messages.py in background
python read_messages.py http://localhost:3000 0 LoadCell1 > pipe0 &
python read_messages.py http://localhost:3000 0 LoadCell2 > pipe1 &

# start main and write_records.py in background
./main 652964 1750 < pipe0 | python write_records.py http://localhost:3000 0 LoadCell1 &
./main 1076702 1750 < pipe1 | python write_records.py http://localhost:3000 0 LoadCell2 &

# wait for any of the background processes to finish
wait -n

# kill the other background processes
kill_all

# wait for the background processes to finish
wait
