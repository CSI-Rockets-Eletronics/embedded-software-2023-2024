#!/bin/bash

set -eo pipefail

./unbind.sh

(python read_messages.py http://localhost:3000 0 LoadCell0 | ./main 652964 1750 | python write_records.py http://localhost:3000 0 LoadCell0) \
& (python read_messages.py http://localhost:3000 0 LoadCell1 | ./main 1076702 1750 | python write_records.py http://localhost:3000 0 LoadCell1)
