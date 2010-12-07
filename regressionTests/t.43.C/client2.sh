#! /bin/sh

good=/tmp/lookup.map
./start_manager -C $1/wrapper1Config -R/tmp -L$good -lservice -lprocess

$rgTestPath/tCclient2 $good
res=$?

if [ $res != 0 ]; then
    echo C Client 1 failed with $res
    exit $res
fi

./stop_manager

exit 0
