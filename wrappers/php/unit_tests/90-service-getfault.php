#!/usr/local/bin/php
<?php

$service = new PlutonService('getfault');

# Should fail as initialization has not been called

$rc = $service->getRequest();
print "Return from getRequest is $rc\n";


if ($service->hasFault()) {
  print "service has fault after missing initialize - good\n";
}
else {
  print "service does NOT have fault after missing initialize - bad\n";
#  exit(1);
}

$ftext = $service->getFault();
$fc = $service->getFaultCode();
$flong = $service->getFaultMessage("LONG", 1);
$fshort = $service->getFaultMessage("SHORT", 0);
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


print "\n Service  getFault test ok\n";
?>
