#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgBinPath/plPing -KoN -c4 -L$good -i0

res=$?
./stop_manager

exit $res
