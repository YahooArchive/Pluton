#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/config -R/tmp -L$good -lprocessStart -lprocessExit

# plutonClientDebug=1 $rgTestPath/tServiceExit $good
$rgTestPath/tServiceExit $good
res1=$?

./stop_manager

[ $res1 -ne 0 ] && exit $res1

c=`grep 'Process Start: system.exit.0.raw' $rgMANAGEROut | wc -l`
if [ $c -ne 1 ]; then
    echo Expected system.exit.0.raw to only be started once, not $c
    exit 1
fi

c=`grep 'Process Start: nobackoff.exit.0.raw' $rgMANAGEROut | wc -l`
if [ $c -ne 2 ]; then
    echo Expected nobackoff.exit.0.raw to be started twice, not $c
    exit 1
fi


exit 0
