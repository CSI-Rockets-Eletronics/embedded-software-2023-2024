#!/bin/bash

set -e

./unbind.sh

(python read_messages.py http://localhost:3000 0 LoadCell0 && pkill -P $$ || exit 1; ./main 652964 1750 && pkill -P $$ || exit 1; python write_records.py http://localhost:3000 0 LoadCell0) \
& (python read_messages.py http://localhost:3000 0 LoadCell1 && pkill -P $$ || exit 1; ./main 1076702 1750 && pkill -P $$ || exit 1; python write_records.py http://localhost:3000 0 LoadCell1)
