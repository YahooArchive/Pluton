#! /bin/sh

# Zero services configured

./start_manager -L/tmp/lookup.map -C $1/zeroConfig -lservice -R/tmp
./stop_manager

grep 'Config Scan Complete: New=0 Retained=0 Replaced=0' $rgMANAGEROut
