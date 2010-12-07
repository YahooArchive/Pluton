#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

ulimit -c 0
$rgTestPath/tClientPollProxyBad $good
res=$?

echo Exit with $res

./stop_manager

[ $res -ne 134 ] && exit 1

exit 0
