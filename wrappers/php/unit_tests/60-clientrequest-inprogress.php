#!/usr/local/bin/php
<?php

$expected = "I expect this to be echo'd out";

$client = new PlutonClient('inprogress');
$client->initialize();

$request = new PlutonClientRequest();
$request->setRequestData($expected);

$request->setContext("echo.sleepMS", "3000");
$client->addRequest('system.echo.0.raw', $request);
$client->executeAndWaitSent();

if ($request->inProgress()) {
  print "Request is in progress - good\n";
}
else {
  print "Request is NOT in progress - bad\n";
  exit(1);
}

$res = $client->executeAndWaitAny();
print "executeAndWaitAny returned $res\n";
if ($request->inProgress()) {
  print "Request is still in progress - bad\n";
  exit(2);
}
else {
  print "Request is NOT in progress - good\n";
}

$responseData = null;
$request->getResponseData($responseData);

if($responseData !== $expected) {
  print "Client test failed\n  expected: $expected\n  received: $responseData\n";
  exit(3);
}
else {
  print "Client test successful\n";
}

?>
