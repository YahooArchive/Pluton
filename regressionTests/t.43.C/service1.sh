#! /bin/sh

# Make sure manager inherits these values

failed=0

good=/tmp/goodlookup.map
./start_manager -C $1/wrapper1Config -R/tmp -L$good -lservice -lprocess

echo Output looks like
echo 'vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv'
$rgBinPath/plSend -L$good wrapper.one.0.raw "qwertyu"
echo '^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^'


$rgBinPath/plSend -L$good wrapper.one.0.raw "qwertyu" | grep "Request Length = 7"
if [ $? -ne 0 ]; then
    echo Error: Expected Request Length of 7
    failed=1
fi

$rgBinPath/plSend -CK12=hello -L$good wrapper.one.0.raw "Cqwertyu" | grep 'Context for K12=hello'
if [ $? -ne 0 ]; then
    echo Error: Expected Context K12=hello
    failed=1
fi

$rgBinPath/plSend -L$good wrapper.one.0.raw "qwertyu" | grep 'SK=wrapper.one.0.r and'
if [ $? -ne 0 ]; then
    echo Error: Expected Reconstruction of service key failed
    failed=1
fi

./stop_manager

exit $failed
