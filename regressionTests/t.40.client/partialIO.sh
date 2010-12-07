#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/singleEchoConfig -R/tmp -L$good

$rgTestPath/tPartialIO $good
res=$?

./stop_manager

exit $res
