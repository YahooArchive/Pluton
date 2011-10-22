#!/usr/local/bin/php
<?php

$expected = "I expect this to be echo'd out";

$client = new PlutonClient('simple');
$client->initialize();

$request = new PlutonClientRequest();
$request->setRequestData($expected);
$request->setClientHandle(23);

$client->addRequest('system.echo.0.raw', $request);
$client->executeAndWaitSent();

if ($request->inProgress()) {
  print "Request is in progress\n";
}
else {
  print "Request is NOT in progress\n";
}

$res = $client->executeAndWaitAny();
print "executeAndWaitAny returned $res\n";
$res = $request->getClientHandle();
if ($res != 23) {
  print "Expected return client handle of 23 rather than $res\n";
  exit(2);
}
print "getClientHandle returned $res\n";

$responseData = null;
$request->getResponseData($responseData);

if($responseData != $expected) {
  print "Client test failed\n  expected: $expected\n  received: $responseData\n";
  exit(3);
}

print "Calling reset\n";
$request->reset();
print "After reset\n";
$res = $request->getClientHandle();
$responseData = null;
print "Getting response data\n";
$request->getResponseData($responseData);

if ($res == 23) {
  print "Did not expect return client handle of 23 after reset\n";
  exit(4);
}

print "After reset handle=$res and response Data=$responseData\n";

?>
