#! /bin/sh

# Test every config parameter without error

./start_manager -L/tmp/lookup.map -C $1/allNoErrorsConfig -R/tmp -lservice
./stop_manager

grep 'New=19' $rgMANAGEROut || exit 1
grep 'Service Error:' $rgMANAGEROut && exit 1

exit 0
