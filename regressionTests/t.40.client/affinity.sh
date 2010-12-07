#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfigAffinity -R/tmp -L$good -lservice -lprocess

$rgTestPath/tClientAffinity $good debug
res=$?
./stop_manager

exit $res
