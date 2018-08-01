#!/bin/sh

opts='--cpus 1 --hash 256m --egtbpath /egtb/three;/egtb/four;/egtb/five'

if [ $# -ne 2 ]; then
  echo "Usage: ${0} <required-test-name> <time>"
  exit 1
fi
./typhoon ${opts} --logfile test.log --batch --command "st ${2}; script ${1}"
tail -19 ./test.log | head -9 > ./checkin
./suite_diff.pl ./test.log ./lastrun.log >> ./checkin
more ./checkin
