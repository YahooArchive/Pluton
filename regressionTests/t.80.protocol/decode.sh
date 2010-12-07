#! /bin/sh

# Test that the packet decoder finds errors

good='0Q,3b100,12aplPing:26666,17csystem.echo.0.raw,4l5000,0z'
bad1='0G,3b100,12aplPing:26666,17csystem.echo.0.raw,4l5000,0z'	# Wrong PT
bad2='0Q,3b1x00,12aplPing:26666,17csystem.echo.0.raw,4l5000,0z' # Malformed
bad3='0Q,3b100,12aplPing:26666,17csystem.echo.0.raw,4l5000,4l5000,0z' # Duplicate
bad4='0Q,999999999993b100,12aplPing:26666,17csystem.echo.0.raw,4l5000,0z' # Large length
bad5='0Q,3b100,12\tplPing:26666,17csystem.echo.0.raw,4l5000,0z' # non-printable type
bad6='xQ,3b100,12aplPing:26666,17csystem.echo.0.raw,4l5000,0z' # Leading must be digit


ret=0
echo $good | $rgServicesPath/echo 3<&0 4>/dev/null
if [ $? -ne 0 ]; then
    echo Expected acceptance of good packet
    ret=1
fi

printf '%s,' $good | $rgServicesPath/echo 3<&0 4>/dev/null
if [ $? -ne 0 ]; then
    echo Expected acceptance of good packet with trailing comma
    ret=1
fi

printf '%sX' $good | $rgServicesPath/echo 3<&0 4>/dev/null
if [ $? -eq 0 ]; then
    echo Expected fault with bad terminator
    ret=1
fi

echo $bad1 | $rgServicesPath/echo 3<&0 4>/dev/null
if [ $? -eq 0 ]; then
    echo Expected fault with bad1 packet
    ret=1
fi

echo $bad2 | $rgServicesPath/echo 3<&0 4>/dev/null
if [ $? -eq 0 ]; then
    echo Expected fault with bad2 packet
    ret=1
fi

echo $bad3 | $rgServicesPath/echo 3<&0 4>/dev/null
if [ $? -eq 0 ]; then
    echo Expected fault with bad3 packet
    ret=1
fi

echo $bad4 | $rgServicesPath/echo 3<&0 4>/dev/null
if [ $? -eq 0 ]; then
    echo Expected fault with bad4 packet
    ret=1
fi

printf $bad5 | $rgServicesPath/echo 3<&0 4>/dev/null
if [ $? -eq 0 ]; then
    echo Expected fault with bad5 packet
    ret=1
fi

echo $bad6 | $rgServicesPath/echo 3<&0 4>/dev/null
if [ $? -eq 0 ]; then
    echo Expected fault with bad6 packet
    ret=1
fi

exit $ret
