#! /bin/sh

echo Bug 1448050

# [ bug 1448050 ] Truncated data returned from service on certain size boundaries

pwd
testDir=$1
testFile=$testDir/1448050.testfile

lup=/tmp/lookup.map
conf=$testDir/1448050.conf
./start_manager -L $lup -C $conf -R/tmp -lall

$rgBinPath/plSend -L $lup <$testFile >$rgData/1448050.result system.echo.0.raw
res=$?

./stop_manager

if [ $res != 0 ]; then
    echo plSend Failed
    exit $res
fi

echo cmp -s $testFile $rgData/1448050.result
cmp -s $testFile $rgData/1448050.result

if [ $? != 0 ]; then
    echo compare fails
    exit 1
fi

