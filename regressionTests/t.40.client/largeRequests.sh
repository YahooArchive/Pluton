#! /bin/sh

export plutonLookupMap=/tmp/lookup.map
./start_manager -C $1/echoConfig -R/tmp -L$plutonLookupMap

failed=
for s in \
      32767   32768   32769 \
      65000 65480 65490 65495 65500 \
      65535   65536   65537 \
     131071  131072  131073 \
     262143  262144  262145 \
     524287  524288  524289 \
     999999 1000000 1000001 \
    1048575 1048576 1048577
do
  $rgBinPath/plPing -c3 -i0 -s$s
  [ $? -eq 0 ] || failed="$failed $s"
done

./stop_manager

if [ -s "$failed" ]; then
    echo Failed : $failed
    exit 1
fi

exit 0
