#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/retryConfig -R/tmp -L$good -lservice -lprocess

res=0

# Tell the service that we want gargage on the next request

$rgBinPath/plSend -Cgarbage=1 -L$good test.retry.0.raw ""
if [ $? -ne 0 ]; then
    echo Exepected good send for garbage did not work
    res=1
fi

# Should get garbage back now

$rgBinPath/plSend -o -L$good test.retry.0.raw ""
if [ $? -eq 0 ]; then
    echo Unexpected good return when wanted garbage
    res=1
fi

# Set up for a sleep return

$rgBinPath/plSend -Csleep=1 -L$good test.retry.0.raw ""
if [ $? -ne 0 ]; then
    echo Exepected good send for sleep did not work
    res=1
fi

# Should time out now

$rgBinPath/plSend -L$good test.retry.0.raw ""
if [ $? -eq 0 ]; then
    echo Unexpected good return when wanted timeout
    res=1
fi

./stop_manager

exit $res
