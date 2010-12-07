#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgTestPath/tClientStateThreads $good
res=$?
./stop_manager

exit $res
