#!/bin/bash

set -eo pipefail

./unbind.sh

python read_messages.py http://localhost:3000 0 IDA100 | ./main | python write_records.py http://localhost:3000 0 IDA100
