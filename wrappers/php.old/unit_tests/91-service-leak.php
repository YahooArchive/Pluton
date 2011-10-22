#!/usr/local/bin/php
<?php

function initService($rc)
{
  for ($ix=0; $ix < $rc; $ix++) {
    $service[$ix] = new PlutonService("leak");
  }
}

function oneRun($rc, $sTotal, $sInternal)
{
  $my_pid = getmypid();
  $ps = `/bin/ps -orss -p$my_pid`;
  list($junk, $psBefore) = explode("\n", $ps, 2);
  initService($rc);
  $eTotal = memory_get_usage(true);
  $eInternal = memory_get_usage(false);

  $diffTotal = $eTotal - $sTotal;
  $diffInternal = $eInternal - $sInternal;

  $ps = `ps -orss -p$my_pid`;
  list($junk, $psAfter) = explode("\n", $ps, 2);
  $psBefore = trim($psBefore);
  $psAfter = trim($psAfter);
  $diffPS = $psAfter - $psBefore;
#  print "$rc: Start (int/total) = $sInternal/$sTotal End = $eInternal/$eTotal ps = $psBefore/$psAfter";
#  print " Diff = $diffInternal/$diffTotal/$diffPS\n";

  return $psAfter;
}

# The first 4-5 iterations tends to grow memory - I assume this a php *thing*, but after that it should stabilize.

$rc = 200;
$depth = 400;
if ($argc > 1) {
  $rc = $argv[1];
}

if ($argc > 2) {
  $depth = $argv[2];
}
print "Repeat Count=$rc Depth=$depth\n";

for ($ix=0; $ix < 40; $ix++) {
  oneRun($depth, memory_get_usage(true), memory_get_usage(false));
}

$st = oneRun($depth, memory_get_usage(true), memory_get_usage(false));

for ($ix=0; $ix < $rc; $ix++) {
  oneRun($depth, memory_get_usage(true), memory_get_usage(false));
}

$en = oneRun($depth, memory_get_usage(true), memory_get_usage(false));
$enInternal = memory_get_usage(false);
$enExternal = memory_get_usage(true);

$diff = $en - $st;
$diffInternal = $enInternal - $stInternal;
$diffExternal = $enExternal - $stExternal;

if (($diff > 100) || ($diff < 0)) {
  print "Scotty - we appear to have a service leak!\n";
  print "ps Start=$st End=$en Diff=$diff\n";
  print "Int Start=$stInternal End=$enInternal Diff=$diffInternal\n";
  print "Ext Start=$stExternal End=$enExternal Diff=$diffExternal\n";
  exit(10);
}

print "Service leak test ok\n";
?>
