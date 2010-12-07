#! /bin/sh

# Test for replaced and new configs discovered

mv -f $1/replaceConfig/three.config.0.raw $1/replaceConfig/three.config.0.notvalid

./start_manager -L/tmp/lookup.map -C $1/replaceConfig -lservice -R/tmp
touch $1/replaceConfig/one.config.0.raw
mv -f $1/replaceConfig/three.config.0.notvalid $1/replaceConfig/three.config.0.raw
./hup_manager
sleep 1
./stop_manager

grep 'Config Scan Complete: New=1 Retained=1 Replaced=1' $rgMANAGEROut
