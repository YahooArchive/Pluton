#! /bin/sh

# Check that the changed semantics of inProgress match the 0.54
# description

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgTestPath/tInProgress $good
res=$?

./stop_manager

exit $res

