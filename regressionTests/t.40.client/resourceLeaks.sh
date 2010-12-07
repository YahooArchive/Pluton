#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig3 -R/tmp -L$good -lservice -lprocess

ulimit -n 40		# fd limits
ulimit -d 4000		# four MB of memory

$rgTestPath/tClientResourceLeaks $good
res=$?
./stop_manager

exit $res
