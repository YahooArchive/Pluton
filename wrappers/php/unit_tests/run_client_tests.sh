#! /bin/sh

errorCount=0
failedList=""
banner='--------------------------------------------------------------'

echo $banner
echo Testing client side
echo $banner

for f in *-client-*.php
do
  echo $banner
  echo $f
  dataname=`echo $f | cut -f1 -d.`
  php $f 2>stderr/$dataname >stdout/$dataname
  exitCode=$?
  echo Exit code for $f = $exitCode
  if [ $exitCode -ne 0 ]; then
      errorCount=`expr $errorCount + 1`
      failedList="$failedList $f"
      echo $f failed: exit code=$exitCode
      continue;
  fi
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

if [ -n "$failedList" ]; then
    echo Failed: $errorCount : $failedList
    exit 1
else
    echo All client tests completed successfully
fi
