#! /bin/sh

set -e

BINPATH=/usr/local/bin
export BINPATH

VARPATH=/usr/local/var/pluton
cd $VARPATH

$BINPATH/plKillOldServices ./rendezvous

$BINPATH/plutonManager -C ./configuration -L ./lookup.map -R ./rendezvous	\
    -o					\
    -c 9990				\
    -l serviceConfig			\
    -l calibrate			\
    -l processStart -lprocessExit -lprocessLog -lprocessUsage
