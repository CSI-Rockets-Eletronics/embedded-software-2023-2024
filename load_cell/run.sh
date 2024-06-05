#!/bin/bash

set -eo pipefail

# kill all child processes on exit
# (https://stackoverflow.com/questions/360201/how-do-i-kill-background-processes-jobs-when-my-shell-script-exits)
trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM EXIT

# remove and make pipes
rm -f  read write
mkfifo read write

# start read_messages.py in background
python read_messages.py http://localhost:3000 0 LoadCell > read &

# start write_messages.py in background
python write_records.py http://localhost:3000 0 LoadCell < write &

# start main in background
./main 999999 3568 < read > write & # TODO change 999999 to the correct serial number

# wait for any of the background processes to finish
wait -n
