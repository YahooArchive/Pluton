#! /bin/sh

# Make sure relative RV is set to real

rm -f /tmp/lookup.map
./start_manager -L/tmp/lookup.map -C . -R.
./stop_manager

rp=`grep 'Manager Rendezvous realpath' $rgMANAGEROut |cut -d' ' -f4`

echo Real Path returned = $rp
echo Current Path = `pwd -P`

[ "$rp" = `pwd -P` ] && exit 0

exit 1
