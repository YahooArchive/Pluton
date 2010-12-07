#! /bin/sh

# One service configured

./start_manager -L/tmp/lookup.map -C $1/oneConfig -lservice -R/tmp
./stop_manager

grep 'Config Scan Complete: New=1 Retained=0 Replaced=0' $rgMANAGEROut
