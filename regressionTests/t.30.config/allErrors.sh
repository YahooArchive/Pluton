#! /bin/sh

# Test every config error

./start_manager -L/tmp/lookup.map -C $1/allErrorsConfig -R/tmp -lservice
./stop_manager

grep 'Invalid boolean' $rgMANAGEROut || exit 1
grep 'Invalid multiplier' $rgMANAGEROut || exit 2
grep 'Invalid number' $rgMANAGEROut || exit 3
grep 'Duplicate parameter' $rgMANAGEROut || exit 4
grep 'Multiplier causes overflow' $rgMANAGEROut || exit 5
grep 'Unknown configuration parameter' $rgMANAGEROut || exit 6
grep 'must not be less than lower limit' $rgMANAGEROut || exit 7
grep 'must not be greater than upper limit' $rgMANAGEROut || exit 8
grep 'minimum-processes must not be greater than maximum-processes' $rgMANAGEROut || exit 9
grep "Must include an 'exec'" $rgMANAGEROut || exit 10
grep 'serialization type is unrecognized' $rgMANAGEROut || exit 11
grep 'Config Warning: Ignoring file' $rgMANAGEROut || exit 12

# Make sure we didn't miss any

ec=`grep 'Config Error:' $rgMANAGEROut|wc -l`
[ $ec -ne 11 ] && exit `expr 100 + $ec`	# Communicate count back to failure tester

exit 0
