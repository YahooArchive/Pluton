#! /bin/sh

# check that the faux data exchanger works

fn=$rgData/dex.$$

input1='0Q,3b100,12aplPing:26666,17csystem.echo.0.raw,4l5000,4kabcd,0z'
output1='0A,3b100,12aplPing:26666,'
output2=',4qabcd,0z'
input2='asdhjwer234234adsfsdafwr234234asdfasdfsa junkosville'

echo $input1 | $rgServicesPath/echo 3<&0 4>$fn.1
if [ $? -ne 0 ]; then
    echo Non-zero exit from echo service using FAUX
    exit 1
fi

grep -q $output1 $fn.1 || exit 2
grep -q $output2 $fn.1 || exit 3

# Now via plTest

echo $input2 | $rgBinPath/plTest $rgServicesPath/echo >$fn.2 2>/dev/null
grep -q "$input2" $fn.2 || exit 4

exit 0
