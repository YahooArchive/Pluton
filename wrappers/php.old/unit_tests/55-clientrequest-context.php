#!/usr/local/bin/php
<?php

function abortme($why, $excode)
{
  print "Context1 failed: $why\n";
  exit($excode);
}

$client = new PlutonClient('context');
$request = new PlutonClientRequest();

$client->initialize();

if (!$request->setContext("random", 23)) {
  abortme("Context set of innocuous name failed", 1);
}

if ($request->setContext("pluton.haha", 24)) {
  abortme("Context set of protected namespace succeeded", 2);
}

$request->reset();
$request->setContext("echo.sleepMS", "3300");

$res = $client->addRequest("system.echo.0.raw", $request);

print "AddRequest = $res\n";
if (!$res) {
  $em1 = $request->getFaultText();
  $em2 = $client->getFault();
  abortme("addRequest Failed with $em1:$em2", 3);
 }


$st = time();

$res = $client->executeAndWaitAll();
print "E&WAll = $res\n";

$et = time();

print "Time diffs st=$st et=$et\n";

if (($st+2) > $et) {
  abortme("Context didn't caused echo to sleep - was it really set?", 3);
}

?>
