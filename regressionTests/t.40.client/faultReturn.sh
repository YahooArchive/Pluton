#! /bin/sh

# Purposely send a bad request to echo to get a fault back

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgBinPath/plSend -L$good -Cecho.sleepMS=-23 system.echo.0.raw ""
res=$?

echo exit code from plSend is $res

./stop_manager

[ $res -ne 1 ] && exit 1
exit 0
