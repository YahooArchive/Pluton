#! /bin/sh

rm -rf stdout stderr dataout
mkdir stdout stderr dataout

./run_client_tests.sh
cec=$?

./run_service_tests.sh
sec=$?

./run_other_tests.sh
oec=$?

if [ $cec -eq 0 ] && [ $sec -eq 0 ] && [ $oec -eq 0 ]; then
    echo All tests completed successfully
    exit 0
else
    echo At least one test failed
fi

exit 1
