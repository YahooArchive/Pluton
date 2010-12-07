#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/config -R/tmp -L$good -lservice -lprocess

failed=0
for s in COBOL HTML JMS JSON NETSTRING PHP SOAP XML raw
do
  k=normal.func1.1.$s
  $rgBinPath/plPing -c1 -L$good $k
  if [ $? != 0 ]; then
      echo Lookup of $k failed
      failed=1
  fi

  k=twild.func1.1.$s
  $rgBinPath/plPing -c1 -L$good $k
  if [ $? != 0 ]; then
      echo Lookup of $k failed
      failed=1
  fi

  k=twild.func51f.1.$s
  $rgBinPath/plPing -c1 -L$good $k
  if [ $? != 0 ]; then
      echo Lookup of $k failed
      failed=1
  fi
done

./stop_manager

exit $failed
