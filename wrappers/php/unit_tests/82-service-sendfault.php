#!/usr/local/bin/php
<?php

$service = new PlutonService('sendfault');
$service->initialize();

$service->getRequest();
$service->sendFault(11, 'sendResponse() failed');

$service->terminate();

exit(0);

?>
