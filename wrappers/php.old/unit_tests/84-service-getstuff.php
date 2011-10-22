#!/usr/local/bin/php
<?php

$service = new PlutonService('getstuff');
$service->initialize();

$service->getRequest();

$sk = $service->getServiceKey();
$sa = $service->getServiceApplication();
$sf = $service->getServiceFunction();
$sv = $service->getServiceVersion();
$serial = $service->getSerializationType();
$cn = $service->getClientName();

$service->sendResponse("sk=$sk sa=$sa sf=$sf sv=$sv serial=$serial cn=$cn\n");

$service->terminate();

# Give plTest a chance to consume the response

sleep(2);

exit(0);

?>
