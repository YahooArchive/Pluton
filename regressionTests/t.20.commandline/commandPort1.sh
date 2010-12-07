#! /bin/sh

# Command port fail due to priviledged

./start_manager -L/tmp/lookup.map -c localhost:22
./stop_manager

grep -q 'Warning: Cannot open Command Port' $rgMANAGEROut || exit 1

exit 0
