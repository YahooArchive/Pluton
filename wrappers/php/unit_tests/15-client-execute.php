#!/usr/local/bin/php
<?php

$expected = "I expect this to be echo'd out";

$client = new PlutonClient('execute');
$client->initialize();

$request10 = new PlutonClientRequest();
$request10->setRequestData($expected);
$request10->setClientHandle(10);

$request2 = new PlutonClientRequest();
$request2->setRequestData($expected);
$request2->setClientHandle(2);

$request3 = new PlutonClientRequest();
$request3->setRequestData($expected);
$request3->setClientHandle(3);

$client->addRequest('system.echo.0.raw', $request10);
$client->addRequest('system.echo.0.raw', $request2);
$client->addRequest('system.echo.0.raw', $request3);

$res = $client->executeAndWaitOne($request2);
print "executeAndWaitOne returned $res\n";
if ($res != 1) {
  print "E&WOne failed. Expected return of 1 rather than $res\n";
  exit(2);
}

print "executeAndWaitOne = $res = ok\n";

$res = $client->executeAndWaitAny();
if (($res != 10) && ($res != 3)) {
  print "E&WAny failed. Expected return of 10 or 3 rather than $res\n";
  exit(2);
}
print "executeAndWaitAny returned 3 or 10 - good\n";

print "executeAndWaitAny first = ok\n";

$res = $client->executeAndWaitAny();
if (($res != 10) && ($res != 3)) {
  print "E&WAny failed. Expected return of 10 or 3 rather than $res\n";
  exit(2);
}
print "executeAndWaitAny returned 10 or 3 - good\n";

print "Second executeAndWaitAny second = ok\n";

?>
