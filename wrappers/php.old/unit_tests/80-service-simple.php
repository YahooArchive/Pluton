#!/usr/local/bin/php
<?php

// Create and initialize the Pluton service.
$service = new PlutonService('simple');
$service->initialize();

$api = $service->getAPIVersion();

print "Service API Version: $api\n";
if (strlen($api) == 0) {
  print "Expected a non-zero length api version\n";
  exit(1);
}

// Invoke the call loop.
while($service->getRequest()) {
  $request = $service->getRequestData();
  if(!$service->sendResponse($request)) {
    $service->sendFault(11, 'sendResponse() failed');
  }
}

if($service->hasFault()) {
  error_log('Service had a fault');
}

$service->terminate();

print "Terminate complete - about to exit\n";

exit(0);

?>
