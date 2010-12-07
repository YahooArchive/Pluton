#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/config -R/tmp -L$good -lservice -lprocess

plutonClientDebug=1 $rgTestPath/tClientEventFilter $good
res=$?
./stop_manager

exit $res

