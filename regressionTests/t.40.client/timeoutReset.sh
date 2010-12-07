#! /bin/sh

export plutonLookupMap=/tmp/lookup.map
./start_manager -C $1/echoConfig -R/tmp -L$plutonLookupMap

# Check the the perCallerClient timeout is reset when the first request is added
# This is easily tested by running plPing for longer than the timeout.

failed=
$rgBinPath/plPing -c10 -t5 -L$plutonLookupMap
res=$?

./stop_manager

echo Result: $res
if [ $res -ne 0 ]; then
    echo Failed : timeoutReset $res
    exit 1
fi

exit 0
