#! /bin/sh

LD_LIBRARY_PATH=$rgTOP/wrappers/perl:$LD_LIBRARY_PATH
PERL5LIB=$rgTOP/wrappers/perl

export LD_LIBRARY_PATH PERL5LIB

good=/tmp/goodlookup.map
./start_manager -C $1/echoConfig -R/tmp -L$good -lservice -lprocess

$rgTestPath/tPerlSwig1.pl $good
res=$?
./stop_manager

exit $res
