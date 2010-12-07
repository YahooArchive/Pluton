#! /bin/sh

# Generate every config warning

# Have to create a bad symlink on the fly as CVS won't check it in

rm -f $1/warningConfig/bad-stat.good.0.raw
ln -s /dev/null/not/here $1/warningConfig/bad-stat.good.0.raw

./start_manager -L/tmp/lookup.map -C $1/warningConfig -R/tmp -lservice
./stop_manager

grep 'Ignoring file' $rgMANAGEROut || exit 1
grep 'Cannot stat' $rgMANAGEROut || exit 3
grep 'Zero exec-failure-backoff' $rgMANAGEROut || exit 2

# Make sure we didn't miss any

ec=`grep 'Config Warning:' $rgMANAGEROut|grep -v 'Ignoring file' | wc -l`
[ $ec -ne 2 ] && exit `expr 100 + $ec`	# Communicate count back to failure tester

exit 0
