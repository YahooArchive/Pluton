#! /bin/sh

# Not implemented
exit 0

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgTestPath/tClientExecuteBlocked $good
res=$?
./stop_manager

exit $res
