#! /bin/sh

# Command port listening

./start_manager -L/tmp/lookup.map -c 127.0.0.1:9991

netstat -an | grep LISTEN | grep 127.0.0.1.9991
res=$?

./stop_manager

exit $res

