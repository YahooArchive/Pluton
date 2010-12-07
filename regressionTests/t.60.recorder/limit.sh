#! /bin/sh

# check that the limit to the number of files is honored.

good=/tmp/goodlookup.map
./start_manager -C $1/configLimit -R/tmp -L$good -lservice -lprocess

$rgBinPath/plPing -c20 -i0 -L$good system.echo.0.raw
./stop_manager

# test that the files cycle within the limit of 3

failed=0
wc=`ls $rgData/limit.in.*|wc -l`
echo Input files: $wc

if [ $wc -ne 3 ]; then
    echo Wrong input file cycle of $wc
    failed=1
fi

wc=`ls $rgData/limit.out.*|wc -l`
echo Output Files: $wc

if [ $wc -ne 3 ]; then
    echo Wrong output file cycle of $wc
    failed=1
fi

exit $failed
