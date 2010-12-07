#! /bin/sh

# Fail due to no lookupmap Path

./start_manager -C . -L nopath/ffff
res=$?
./stop_manager

[ $res -eq 0 ] && exit 1
exit 0
