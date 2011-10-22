#!/usr/local/bin/php
<?php

$client = new PlutonClient('fault');
$client->initialize("/does/not/exist");

if ($client->hasFault()) {
  print "client has fault after bogus initialize - good\n";
}
else {
  print "client does NOT have fault after bogus initialize - bad\n";
  exit(1);
}

$ftext = $client->getFault();
$fc = $client->getFaultCode();
$flong = $client->getFaultMessage("LONG", 1);
$fshort = $client->getFaultMessage("SHORT", 0);
print "Fault code=$fc Text=$ftext\n";
print "Short=$fshort\n";
print "Long=$flong\n";

if (strlen($flong) == 0) {
  print "Long message should not be zero length\n";
  exit(3);
}

if (strlen($fshort) == strlen($flong)) {
  print "Hmm expected short message to be shorter than long message\n";
  exit(4);
}

$client->initialize();
$client->reset();
if ($client->hasFault()) {
  print "client has fault after reset - bad\n";
  exit(5);
}

$request = new PlutonClientRequest();
$request->setRequestData("Some data");

$client->addRequest('system.echo.0.raw', $request);
if ($request->inProgress()) {
  print "Request is in progress - good\n";
}
else {
  print "Request is NOT in progress - bad\n";
  exit(6);
}

$client->reset();

if ($request->inProgress()) {
  print "Request is in progress - bad\n";
  exit(7);
}
else {
  print "Request is NOT in progress - good\n";
}

print "\nClient fault test ok\n";
?>
