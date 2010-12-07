#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgTestPath/tClientPollProxy -c20 -p4 -d -L $good
res=$?
./stop_manager

exit $res
