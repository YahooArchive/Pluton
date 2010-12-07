#! /bin/sh

$rgTestPath/tThread3 ss
[ $? -eq 0 ] && exit 1
echo Grepping for assert two
grep 'ssertion `setThreadHandlersNeverCalled == true' $rgTESTErr
res=$?
echo After grep2 = $res
[ $res -ne 0 ] && exit 2

$rgTestPath/tThread3 cs
echo Grepping for assert three
grep 'ssertion `tooLateToSetThreadHandlers == false' $rgTESTErr
[ $? -ne 0 ] && exit 3

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgTestPath/tThread4 $good

./stop_manager

echo Grepping for assert four
grep 'ssertion `_oneAtATimePerCaller == false' $rgTESTErr
[ $? -ne 0 ] && exit 4


good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgTestPath/tThread5 $good

./stop_manager

echo Grepping for assert five
grep 'ssertion `_pcClient->threadOwnerUnchanged(thread_self' $rgTESTErr
[ $? -ne 0 ] && exit 5

exit 0
