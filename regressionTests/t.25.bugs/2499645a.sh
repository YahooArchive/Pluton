#! /bin/sh

echo Bug 2499645a

# [ bug 2499645 ] Service stall on shutdown with request pending

# To create this scenario:
#
#	1. Service must wedge between sendResponse() and getRequest()
#
#	2. Client must send a request with a large timeout so that it sits
#	   on the accept queue for the duration of the Manager orderly
#	   shutdown limit.
#
#	3. Shutdown Manager and see if it exists prior to the end of the client
#	   request.
#
# The fix is for the manager to give up after a certain time.

lup=/tmp/lookup.map
conf=$1/2499645.conf
echo conf = $conf

./start_manager -L $lup -C $conf -R/tmp -lall

echo Manager started
date
sleep 2

# Get the service to wedge after the next request

$rgBinPath/plSend -L $lup -Cecho.sleepAfter=35 -Cecho.log=1 system.echo.0.raw ""
res=$?
if [ $res -ne 0 ]; then
    echo Initial ping failed - giving up. ec=$res
    ./stop_manager
    exit 1
fi

$rgBinPath/plSend -L $lup -t35 -Cecho.log=1 system.echo.0.raw "" &
pingPid=$!

if [ $res -ne 0 ]; then
    echo Wedge ping failed - giving up. ec=$res
    ./stop_manager
    kill $pingPid
    exit 2
fi

./term_manager
sleep 33		# Manager default emergency exit is 30 seconds

ee=`grep 'Emergency Exit' $rgMANAGEROut |wc -l`

[ $ee -ne 1 ] && exit 3

wait $pingPid

exit 0
