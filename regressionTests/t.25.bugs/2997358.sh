#! /bin/sh

bug=2997358
echo Bug $bug

# [ bug 2997358 ] clientRequest::reset() needs to reset internal state

lup=/tmp/lookup.map
conf=$1/$bug.conf
echo conf = $conf

./start_manager -L $lup -C $conf -R/tmp -lall

$rgTestPath/tClientReset $lup
res=$?
./stop_manager

exit $res
