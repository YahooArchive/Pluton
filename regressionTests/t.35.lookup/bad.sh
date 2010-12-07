#! /bin/sh

good=/tmp/goodlookup.map
./start_manager -C $1/config -R/tmp -L$good -lservice -lprocess

# Most of the serviceKey tests are actually done in tClientErrors.cc
# so this is somewhat redundant.

failed=0
for k in normal normal. normal.func1 normal.func1. \
    normal.func1.1.COBO normal.func1.1.COB normal.func1.1.CO normal.func1.1.C \
    norma.func1.1.COBOL normal.func.1.COBOL normal.unc1.1.COBOL normal.func1.1.OBOL
do
  $rgBinPath/plPing -c1 -L$good $k
  if [ $? = 0 ]; then
      echo Lookup of $k unexpectedly worked
      failed=1
  fi
done

for s in COBOL HTML JMS JSON NETSTRING PHP SOAP XML raw badserial
do
  k=normal.func1.2.$s
  $rgBinPath/plPing -c1 -L$good $k
  if [ $? = 0 ]; then
      echo Lookup of $k unexpectedly worked
      failed=1
  fi

  k=twild.func1.2.$s
  $rgBinPath/plPing -c1 -L$good $k
  if [ $? = 0 ]; then
      echo Lookup of $k unexpectedly worked
      failed=1
  fi

  k=twild.func51f.2.$s
  $rgBinPath/plPing -c1 -L$good $k
  if [ $? = 0 ]; then
      echo Lookup of $k unexpectedly worked
      failed=1
  fi
done

./stop_manager

exit $failed
