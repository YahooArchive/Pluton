#! /bin/sh

# This is a redundant test - see t.47

ulimit -c 0
$rgTestPath/tClientGlobals1
res=$?

[ $res -ne 134 ] && exit 1
grep 'setThreadHandlersNeverCalled == true' $rgTESTErr || exit 2

exit 0
