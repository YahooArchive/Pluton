#! /bin/sh

export plutonLookupMap=/tmp/lookup.map
./start_manager -C $1/echoConfig -R/tmp -L$plutonLookupMap

failed=
for s in 0 1 2 3 4 5 6 10 11 20 100 101 201 202 998 \
    999 1000 1001 1023 1024 1025 \
    1099 2000 2001 2047 2048 2049 \
    3099 4000 4001 4095 4096 4097 \
    8191 8192 8193 \
    16383 16384 16385 \
    32767 32768 32769
do
  $rgBinPath/plPing -c1 -s$s
  [ $? -eq 0 ] || failed="$failed $s"
done

./stop_manager

if [ -s "$failed" ]; then
    echo Failed : $failed
    exit 1
fi

exit 0
