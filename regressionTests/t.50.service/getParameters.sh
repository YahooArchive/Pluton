#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/config -R/tmp -L$good -lservice -lprocess

$rgBinPath/plSend -L$good test.service1.3.HTML ''
res1=$?

$rgBinPath/plSend -L$good test.wildcard.3.JSON ''
res2=$?

./stop_manager

exit `expr $res1 + $res2`
