#! /bin/sh

export plutonLookupMap=/tmp/lookup.map
./start_manager -C $1/echoConfig -R/tmp -L$plutonLookupMap

# ExecuteAndWaitOne() was incorrectly re-constructing the oTodo queue
# when deleting the completed request. Symptoms visible in a multi-add request
# where the request instances are re-used.

failed=
$rgBinPath/plPing -p4 -c5 -L$plutonLookupMap
res=$?

./stop_manager

echo Result: $res
if [ $res -ne 0 ]; then
    echo Failed : waitOneBug still present
    exit 1
fi

exit 0
