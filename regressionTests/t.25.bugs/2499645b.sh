#! /bin/sh

echo Bug 2499645b

# [ bug 2499645 ] unresponsiveness test not working if service wedged after response
#		  but before request
# To create this scenario:
#
#	1. Service must wedge between sendResponse() and getRequest()
#
# The fix is to disassociate the response timers from the "ready to
# accept" flag in shm.

lup=/tmp/lookup.map
conf=$1/2499645.conf
echo conf = $conf

./start_manager -L $lup -C $conf -R/tmp -lall

echo Manager started
date
sleep 2

# Get the service to wedge on this request for longer than unresponsive-timeout


$rgBinPath/plSend -L $lup -Cecho.sleepMS=10000 -t15 -Cecho.sleepAfter=55 -Cecho.log=1 system.echo.0.raw ""
res=$?
if [ $res -ne 0 ]; then
    echo Initial ping failed - giving up. ec=$res
    ./stop_manager
    exit 1
fi

sleep 60		# Wait for unresponseness to be detected by the Manager

echo `date` Stopping Manager
./stop_manager

echo `date` Checking logs

ee=`grep 'unresponsive' $rgMANAGEROut |wc -l`

[ $ee -ne 1 ] && exit 3

wait $pingPid

exit 0
