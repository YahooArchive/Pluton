#!/usr/local/bin/php
<?php

$expected = "I expect this to be echo'd out";

$client = new PlutonClient('fault');
$client->initialize();

$request = new PlutonClientRequest();
$request->setRequestData($expected);

$request->setContext("echo.sleepMS", "-3000");	 # We know that echo barfs at this
$client->addRequest('system.echo.0.raw', $request);
$client->executeAndWaitAll();

if ($request->hasFault()) {
  print "Request has fault - good\n";
}
else {
  print "Request does not has fault - bad\n";
  exit(1);
}

$fc = $request->getFaultCode();
if ($fc != 111) {
  print "Expected a fault code of 111 rather than $fc\n";
  exit(2);
}

$ftext = $request->getFaultText();
$service = $request->getServiceName();

print "Fault code = $fc\n";
print "Fault text = $ftext\n";
print "service = $service\n";

?>
