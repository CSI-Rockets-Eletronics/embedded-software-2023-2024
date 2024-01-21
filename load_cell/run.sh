#!/bin/bash

set -eo pipefail

./unbind.sh

(
    set -eo pipefail
    mkfifo pipe0
    python read_messages.py http://localhost:3000 0 LoadCell0 > pipe0 & READ_PID=$!
    ./main 652964 1750 < pipe0 | python write_records.py http://localhost:3000 0 LoadCell0
    kill $READ_PID
    rm pipe0
) & (
    set -eo pipefail
    mkfifo pipe1
    python read_messages.py http://localhost:3000 0 LoadCell1 > pipe1 & READ_PID=$!
    ./main 1076702 1750 < pipe1 | python write_records.py http://localhost:3000 0 LoadCell1
    kill $READ_PID
    rm pipe1
)
