#! /bin/sh

# Fail due to no rendezvous directory

./start_manager -L/tmp/lookup.map -C . -R nopath
res=$?
./stop_manager

[ $res -eq 0 ] && exit 1
exit 0
