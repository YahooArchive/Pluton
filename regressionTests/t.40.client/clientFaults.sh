#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good

$rgTestPath/tClientFault1 /dev/null $good
res=$?

./stop_manager

exit $res
