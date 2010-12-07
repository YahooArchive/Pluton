#!/usr/local/bin/php
<?php

$service = new PlutonService('context');
$service->initialize();

$service->getRequest();

$ctx = $service->getContext("ctx");
$service->sendResponse($ctx);

$service->terminate();

# Give plTest a chance to consume the response

sleep(2);

exit(0);

?>
