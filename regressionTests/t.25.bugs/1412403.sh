#! /bin/sh

echo Bug 1412403

# [ bug 1412403 ] A service reload can mis-count remaining processes

lup=/tmp/lookup.map
conf=$1/1412403.conf
echo conf = $conf

./start_manager -L $lup -C $conf -R/tmp -lall

echo Manager started
date
sleep 2

# Make sure the service is active by pinging it

$rgBinPath/plPing -L $lup -c1 -Cecho.sleepMS=40000 -t 1
res=$?
if [ $res -ne 0 ]; then
    echo Initial ping failed - giving up
    ./stop_manager
    exit 1
fi

# Now change the config and cause a reload

touch $conf/*

./hup_manager

sleep 2

# Ping again to get the new service

$rgBinPath/plPing -L $lup -c1
res=$?
if [ $res -ne 0 ]; then
    ./stop_manager
    exit 2
fi

./stop_manager

# Count how many times the manager started. One is good, more than one is bad

rp=`grep 'Manager ready' $rgMANAGEROut |wc -l`

[ $rp -ne 1 ] && exit 3

grep -q 'Segmentation fault' $rgMANAGEROut && exit 4

grep -q 'Final Objects: Map=0 Services=0 Process=0 Children=0 Zombies=0' $rgMANAGEROut && exit 0

exit 5
