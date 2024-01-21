#!/bin/bash

set -eo pipefail

./unbind.sh

# start read_messages.py in background
python read_messages.py http://localhost:3000 0 LoadCell0 | ./main  652964 1750 < pipe0 | python write_records.py http://localhost:3000 0 LoadCell0 &
python read_messages.py http://localhost:3000 0 LoadCell1 | ./main 1076702 1750 < pipe1 | python write_records.py http://localhost:3000 0 LoadCell1 &

# wait for all background processes to finish
wait
