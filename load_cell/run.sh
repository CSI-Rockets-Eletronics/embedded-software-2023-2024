#!/bin/bash

set -eo pipefail

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

(
    set -eo pipefail
    # start read_messages.py in background
    python read_messages.py http://localhost:3000 0 LoadCell0 > pipe0 & READ_PID_0=$!
    # start main and write_records.py in foreground
    ./main 652964 1750 < pipe0 | python write_records.py http://localhost:3000 0 LoadCell0
    # kill read_messages.py
    kill $READ_PID
) & (
    set -eo pipefail
    # start read_messages.py in background
    python read_messages.py http://localhost:3000 0 LoadCell1 > pipe1 & READ_PID=$!
    # start main and write_records.py in foreground
    ./main 1076702 1750 < pipe1 | python write_records.py http://localhost:3000 0 LoadCell1
    # kill read_messages.py
    kill $READ_PID
)
