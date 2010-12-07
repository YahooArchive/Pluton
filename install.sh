#! /bin/sh

INSTALLCMD=$1
BINDIR=$2/bin
LIBDIR=$2/lib
RUNDIR=$3

RUNUSER=${4:-`id -nu`}
RUNGROUP=${5:-`id -ng`}

OWNER=root
GROUP=wheel

echo Installing pluton library into $LIBDIR using $INSTALLCMD
echo Install OWNER=$OWNER GROUP=$GROUP
echo Run RUNUSER=$RUNUSER RUNGROUP=$RUNGROUP

$INSTALLCMD -D -m a=r -o $OWNER -g $GROUP clientServiceLibrary/libpluton.so.0.1 $LIBDIR/libpluton.so.0.1
ln -fs $LIBDIR/libpluton.so.0.1 $LIBDIR/libpluton.so
    

echo Installing plutonManager into $BINDIR using $INSTALLCMD

$INSTALLCMD -D -m a=rx -o $OWNER -g $GROUP manager/plutonManager $BINDIR/plutonManager


echo Installing pluton commandline programs into $BINDIR using $INSTALLCMD

for c in plPing plLookup plSend plBatch plVersion plNetStringGrep plTest plKillOldServices
do
  $INSTALLCMD -D -m a=rx -o $OWNER -g $GROUP commands/$c $BINDIR/$c
  echo $c
done


echo Creating rendezvous and configuration area in $RUNDIR

$INSTALLCMD -d -m u=rwx,go=rx -o $RUNUSER -g $RUNGROUP $RUNDIR/pluton
$INSTALLCMD -d -m u=rwx,go=rx -o $RUNUSER -g $RUNGROUP $RUNDIR/pluton/rendezvous
$INSTALLCMD -d -m u=rwx,go=rx -o $RUNUSER -g $RUNGROUP $RUNDIR/pluton/configuration


echo Installing default pluton services into $BINDIR using $INSTALLCMD

for s in curl echo
do
  sname=system.$s.0.raw
  $INSTALLCMD -D -m a=rx -o $OWNER -g $GROUP services/$s $BINDIR/$sname
  rm -f $RUNDIR/pluton/configuration/$sname
  echo "exec $BINDIR/$sname" >$RUNDIR/pluton/configuration/$sname
  chown $OWNER:$GROUP $RUNDIR/pluton/configuration/$sname
  echo $sname
done

echo
echo Done

