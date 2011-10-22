#! /bin/sh

errorCount=0
failedList=""
banner='--------------------------------------------------------------'

echo $banner
echo Testing service side
echo $banner

for f in *-service-*.php
do
  echo $banner
  testname=`echo $f | cut -f1 -d.`
  echo Testing $testname
  for d in $testname.*.data
  do
    echo Testing $f with $d
    dataname=`echo $d | cut -f1-2 -d.`
    rm -f $dataname.actual
    context=""
    [ -r $dataname.context ] && context=`cat $dataname.context`
    plTest $context -i $d -o dataout/$dataname -- php $f 2>stderr/$dataname >stdout/$dataname
    exitCode=$?
    echo Exit code from plTest = $exitCode

    # Non-zero exit codes are ok - stdout/stderr comparisons decide errors

    if [ -r $dataname.expected.dataout ]; then
	echo Running: cmp -s dataout/$dataname $dataname.expected.dataout
	cmp -s dataout/$dataname $dataname.expected.dataout
	if [ $? -ne 0 ]; then
	    errorCount=`expr $errorCount + 1`
	    failedList="$failedList $f"
	    echo $f failed: output differs from expected
	    continue;
	fi
    fi

    if [ -r $dataname.expected.stdout ]; then
	echo Running: cmp -s stdout/$dataname $dataname.expected.stdout
	cmp -s stdout/$dataname $dataname.expected.stdout
	if [ $? -ne 0 ]; then
	    errorCount=`expr $errorCount + 1`
	    failedList="$failedList $f"
	    echo $f failed: output differs from expected
	    continue;
	fi
    fi

    if [ -r $dataname.expected.stderr ]; then
	echo Running: cmp -s stderr/$dataname $dataname.expected.stderr
	cmp -s stderr/$dataname $dataname.expected.stderr
	if [ $? -ne 0 ]; then
	    errorCount=`expr $errorCount + 1`
	    failedList="$failedList $f"
	    echo $f failed: output differs from expected
	    continue;
	fi
    fi
    
    echo $f with $d Ok

  done
done

if [ -n "$failedList" ]; then
    echo Failed: $errorCount : $failedList
    exit 1
else
    echo All service tests completed successfully
fi
