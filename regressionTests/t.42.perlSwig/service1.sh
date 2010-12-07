#! /bin/sh

# Make sure manager inherits these values

export LD_LIBRARY_PATH=$rgTOP/wrappers/perl:$LD_LIBRARY_PATH
export PERL5LIB=$rgTOP/wrappers/perl

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgBinPath/plSend -L$good perl.echo.0.raw "qwertyu" | grep '7 bytes'
res=$?

./stop_manager

exit $res
