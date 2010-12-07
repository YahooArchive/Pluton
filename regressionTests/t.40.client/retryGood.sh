#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/retryConfig -R/tmp -L$good -lservice -lprocess

res=0

# Tell the service that we want garbage on the next request

$rgBinPath/plSend -Cgarbage=1 -L$good test.retry.0.raw ""
if [ $? -ne 0 ]; then
    echo Exepected good send for garbage did not work
    res=1
fi

# Should get garbage on the first attempt and good on the retry

$rgBinPath/plSend -L$good test.retry.0.raw ""
if [ $? -ne 0 ]; then
    echo Unexpected bad return from garbage retry
    res=1
fi

./stop_manager

# Doublecheck that garbage was indeed sent to us

grep -q 'Sending garbage' $rgMANAGEROut || exit 1

exit $res
