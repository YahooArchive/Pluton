#! /bin/sh

# Fail due to no config directory

./start_manager -L/tmp/lookup.map -C nothere
res=$?
./stop_manager

[ $res -eq 0 ] && exit 1
exit 0
