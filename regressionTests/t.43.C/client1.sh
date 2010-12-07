#! /bin/sh

good=/tmp/lookup.map
./start_manager -C $1/wrapper1Config -R/tmp -L$good -lservice -lprocess

echo Starting $rgTestPath/tCclient1 with $LD_LIBRARY_PATH

for d in `echo $LD_LIBRARY_PATH`
do
echo SOs in $d
ls -l $d/*so*
done

plutonClientDebug=1 $rgTestPath/tCclient1 $good
res=$?

if [ $res != 0 ]; then
    echo C Client 1 failed with $res
    exit $res
fi

./stop_manager

exit 0
