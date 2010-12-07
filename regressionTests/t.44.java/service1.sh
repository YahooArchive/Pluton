#! /bin/sh

# Basic service tests

[ `uname` != "Linux" ] && exit 0

good=/tmp/lookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgBinPath/plPing -c5 -L$good system.java.0.raw
res=$?

./stop_manager

[ $res -ne 0 ] && exit $res

exit 0
