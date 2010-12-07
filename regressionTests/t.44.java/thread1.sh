#! /bin/sh

# Basic client tests

[ `uname` != "Linux" ] && exit 0

good=/tmp/lookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

plutonClientDebug=1 /usr/local/bin/java	\
    -Djava.library.path=platform-jni	\
    -cp platform-jar/pluton.jar:$rgTestClasses tJavaThreads1 $good

res=$?
./stop_manager

exit $res
