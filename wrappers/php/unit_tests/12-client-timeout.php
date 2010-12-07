#!/usr/local/bin/php
<?php

$expected = "I expect this to be echo'd out";

$client = new PlutonClient('timeout');
$client->initialize();
$client->setTimeoutMilliSeconds(1000);
$to = $client->getTimeoutMilliSeconds();
if ($to != 1000) {
  print "Expected timeout to be 1000 rather than $to\n";
  exit(1);
}

$request = new PlutonClientRequest();
$request->setRequestData($expected);

$request->setContext("echo.sleepMS", "3000");
$client->addRequest('system.echo.0.raw', $request);
$res = $client->executeAndWaitAll();

print "Result of E&W with timeout is $res\n";

if ($request->hasFault()) {
  print "Request has fault - good\n";
}
else {
  print "Request does NOT have fault - bad\n";
  exit(2);
}

$fc = $request->getFaultCode();

# We happen to know that timeout is -19

if ($fc != -19) {
  print "Expected fault code of -19 rather than $fc\n";
  exit(4);
}

?>
