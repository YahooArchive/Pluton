#! /bin/sh

# Basic service tests

[ `uname` != "Linux" ] && exit 0

JAVA_OPTS=-Djava.library.path=../wrappers/java/jni/i386-rhel4-gcc3
export JAVA_OPTS

good=/tmp/lookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgBinPath/plPing -c1 -L$good -Cctx=sendfault system.java.1.raw
res1=$?
echo Exit code of plPing is $res1

grep 'Fault: 123' $rgTESTErr
res2=$?
echo Grep returned $res2

./stop_manager

[ $res1 -ne 1 -a $res2 -ne 0 ] && exit 1

exit 0
