#! /bin/sh

# Fail due to not being able to write to rendezvous

./start_manager -C . -R /dev -L/tmp/lookup.map
res=$?
./stop_manager

[ $res -eq 0 ] && exit 1
exit 0
