#! /bin/sh

echo Bug 1552859

# [ bug 1552859 ] "Mutex lock" not cleared on waitAndSend() when request queue is empty

pwd

lup=/tmp/lookup.map
conf=$1/1552859.conf
./start_manager -L $lup -C $conf -R/tmp -lall

$rgTestPath/bug1552859 $lup
res=$?

./stop_manager

if [ $res != 0 ]; then
    echo bug1552859 Failed
    exit $res
fi

exit 0

