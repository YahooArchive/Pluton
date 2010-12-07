#! /bin/sh

pwd

if [ -z "$PLATFORM" ]; then
    echo ERROR: PLATFORM environment variable must not be zero length
    exit 1
fi

export rgTOP=`pwd`/..
export rgBinPath=$rgTOP/commands/$PLATFORM
export rgTestPath=$rgTOP/testPrograms/$PLATFORM
export rgTestClasses=$rgTOP/testsPrograms
export rgServicesPath=$rgTOP/services/$PLATFORM
export rgData=`pwd`/data

if [ ! -d $rgBinPath ]; then
    echo ERROR: rgBinPath of $rgBinPath does not exists - is PLATFORM correct?
    exit 1
fi

if [ ! -d $rgTestPath ]; then
    echo ERROR: rgTestPath of $rgTestPath does not exists - is PLATFORM correct?
    exit 1
fi

if [ ! -d $rgServicesPath ]; then
    echo ERROR: rgServicesPath of $rgServicesPath does not exists - is PLATFORM correct?
    exit 1
fi

if [ -z "$LD_LIBRARY_PATH" ]; then
    LD_LIBRARY_PATH=$rgTOP/lib/$PLATFORM:$rgTOP/wrappers/C/$PLATFORM:$rgTOP/wrappers/java/jni/$PLATFORM
    export LD_LIBRARY_PATH
else
    echo Note well: LD_LIBRARY_PATH is not being set to the test libraries
fi

if [ -n "$LD_LIBRARY_PATH" ]; then
    LD_LIBRARY_PATH=$rgTOP/lib/$PLATFORM:$rgTOP/wrappers/C/$PLATFORM:$rgTOP/wrappers/java/jni/$PLATFORM
    export LD_LIBRARY_PATH
else
    echo Note well: LD_LIBRARY_PATH is not being set to the test libraries
fi

echo rgBinPath=$rgBinPath
echo rgTestPath=$rgTestPath
echo rgTestClasses=$rgTestClasses
echo rgServicesPath=$rgServicesPath
echo rgData=$rgData
echo LD_LIBRARY_PATH=$LD_LIBRARY_PATH

# LD_PRELOAD=$rgTOP/lib/$PLATFORM/libpluton.so
# export LD_PRELOAD

STDOUT=$PLATFORM/stdout
STDERR=$PLATFORM/stderr

rm -rf $STDOUT $STDERR data *.core
mkdir -p $STDOUT $STDERR data

# Service and test paths are platform dependent, but configs have
# hard-coded exec paths. Create symlinks for the hard-codes.

rm -f platform-services platform-tests platform-jar platform-jni

ln -sf $rgServicesPath platform-services
ln -sf $rgTestPath platform-tests
ln -sf $rgTOP/wrappers/java/jar platform-jar
ln -sf $rgTOP/wrappers/java/jni/$PLATFORM platform-jni

ulimit -c 2	# Don't really want cores

failed=""
dirlist=$1
[ "$dirlist" ] && shift
testlist=$*
[ -z "$dirlist" ] && dirlist=`ls -d t.*.*| sort -t. -k2n -k3`

echo
echo Regression List: $dirlist

for d in $dirlist
do
  D=`cd $d && pwd`
  D=`basename $D`
  [ -z "$testlist" ] && testlist=`(cd $D; ls *.sh)`
  echo
  echo Tests in $D are: $testlist
  for t in $testlist
    do
    printf "Running %s/%s..." "$D" "$t"
    export rgMANAGEROut=`pwd`/$STDOUT/$D-$t.manager
    export rgMANAGERErr=`pwd`/$STDERR/$D-$t.manager
    export rgTESTOut=`pwd`/$STDOUT/$D-$t.test
    export rgTESTErr=`pwd`/$STDERR/$D-$t.test
    sh $D/$t $D >$rgTESTOut 2>$rgTESTErr
    res=$?
    if [ $res -ne 0 ]; then
	printf " failed $res\n"
	failed="$failed $D/$t"
    else
	printf " ok\n";
    fi
  done
  testlist=""
  echo All of $D done.
done

echo
if [ -n "$failed" ]; then
    echo PLATFORM FAILED LIST: $PLATFORM: $failed
    exit 1
fi

echo All Regression tests worked
