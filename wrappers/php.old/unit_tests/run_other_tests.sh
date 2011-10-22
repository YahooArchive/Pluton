#! /bin/sh

errorCount=0
failedList=""
banner='--------------------------------------------------------------'

echo $banner
echo Testing other stuff
echo $banner

for f in *-other-*.sh
do
  echo $banner
  echo $f
  sh $f
  exitCode=$?
  if [ $exitCode -ne 0 ]; then
      errorCount=`expr $errorCount + 1`
      failedList="$failedList $f"
      echo $f failed: exit code=$exitCode
  else
      echo $f Ok
  fi
done

if [ -n "$failedList" ]; then
    echo Failed: $errorCount : $failedList
    exit 1
else
    echo All other tests completed successfully
fi
