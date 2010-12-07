#! /bin/sh

# check that the various ulimits are honored.

set -x

good=/tmp/goodlookup.map
./start_manager -C $1/config -R/tmp -L$good -lservice -lprocess

$rgBinPath/plPing -c1 -L$good cpu.echo.0.raw
$rgBinPath/plPing -c1 -L$good files.echo.0.raw
$rgBinPath/plPing -c1 -L$good memory.echo.0.raw

./stop_manager

grep -q 'SIGXCPU' $rgMANAGEROut || exit 1
grep -q 'may have exceeded CPU' $rgMANAGEROut || exit 11
grep -q 'may have exceeded Memory' $rgMANAGEROut || exit 13

grep -q 'abnormal cpu.echo.0.raw' $rgMANAGEROut || exit 21
grep -q 'abnormal files.echo.0.raw' $rgMANAGEROut || exit 22
grep -q 'abnormal memory.echo.0.raw' $rgMANAGEROut || exit 23

exit 0
