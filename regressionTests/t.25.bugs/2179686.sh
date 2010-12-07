#! /bin/sh

echo Bug 2179686

# [ bug 2179686 ] The unresponsive-timeout config parameter doesn't work

lup=/tmp/lookup.map
conf=$1/2179686.conf
echo conf = $conf

./start_manager -L $lup -C $conf -R/tmp -lall -dprocess

echo Make sure the service is active by pinging it
$rgBinPath/plPing -L $lup -c1 -t1
res=$?
echo First ping exit code $res
if [ $res -ne 0 ]; then
    ./stop_manager
    exit 1
fi

echo Now make plPing sleep for longer than the unresponsive-timeout value

date
$rgBinPath/plPing -L $lup -c1 -t1 -C10000
res=$?
sleep 3	# Make sure manager notices before we shut it down
date
echo Results from second ping is $res
if [ $res -eq 0 ]; then
    ./stop_manager
    exit 2
fi

sleep 10
echo Make sure the service is active by pinging it
$rgBinPath/plPing -L $lup -c1 -t1
res=$?
echo Results from third ping is $res
if [ $res -ne 0 ]; then
    ./stop_manager
    exit 3
fi

./stop_manager

echo Check that the manager noticed and killed the service.

grep -q 'unresponsive' $rgMANAGEROut && exit 0

exit 5

