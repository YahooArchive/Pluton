#! /bin/sh

# Check that replaying a request via non-manager mode produces the
# same results.

good=/tmp/goodlookup.map
./start_manager -C $1/configRecord -R/tmp -L$good -lservice -lprocess

# test that recording works
$rgBinPath/plSend -L$good -Cset=context system.echo.0.raw "Test Data" >/dev/null

$rgBinPath/plPing -L$good -s100 -c1
$rgBinPath/plPing -L$good -s1000 -c1
$rgBinPath/plPing -L$good -s10000 -c1
$rgBinPath/plPing -L$good -s100000 -c1
$rgBinPath/plPing -L$good -s1000000 -c1
$rgBinPath/plPing -L$good -s10000000 -c1

./stop_manager

infile=`echo $rgData/record.in.*.00001`
outfile=`echo $rgData/record.out.*.00001`

if [ ! -r $infile -o ! -r $outfile ]; then
    echo $infile or $outfile not created
    exit 1
fi

# run echo manually to ensure the same output

for f in 00001 00002 00003 00004 00005 00006 00007
do
  in=`echo $rgData/record.in.*.$f`
  out=`echo $rgData/record.out.*.$f`
  $rgServicesPath/echo 3<$in 4>$rgData/manual.$f
  $rgBinPath/plNetStringGrep -v sa <$rgData/manual.$f >$rgData/manual.strip.$f
  $rgBinPath/plNetStringGrep -v as <$out >$rgData/out.strip.$f

  if [ ! -s $rgData/manual.strip.$f ]; then
      echo Failed: Zero length string: $rgData/manual.strip.$f
      failed=1
  fi

  cmp -s $rgData/manual.strip.$f $rgData/out.strip.$f
  if [ $? -ne 0 ]; then
      echo Failed: cmp -s $rgData/manual.strip.$f $rgData/out.strip.$f
      failed=1
  fi
done

exit $failed
