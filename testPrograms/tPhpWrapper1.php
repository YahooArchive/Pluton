#!/usr/bin/php
<?php

$pc = new PlutonClient("phpClient");
$pc->initialize();

$clReq1 = new PlutonClientRequest();
$clReq2 = new PlutonClientRequest();

$req1 = "1234567890";
$req2 = "qwertyuiop";

$clReq1->setRequestData($req1);
if(!($pc->addRequest(SERVICE_KEY, $clReq1))) {
    print_r("addRequest Error! " . $pc->getFault() . "\n");
    exit(-1);
}

$clReq2->setRequestData($req2);
if(!($pc->addRequest(SERVICE_KEY, $clReq2))) {
    print_r("addRequest Error! " . $pc->getFault() . "\n");
    exit(-1);
}

// ExecuteAndWaitALL
$pc->executeAndWaitAll();
if ($pc->hasFault()) {
    print_r("Fault! " . $pc->getFault() . "\n");
    exit(-1);
}

$response = '';

$fault = $clReq1->getResponseData($response);
if ($fault != 0) {
    print_r("fault: " . $clReq1->getFaultText() . "\n");
    exit(1);
}
if ($response != $req1) {
    print_r("Req1 output is not same as input! [$response] vs [$req1]\n");
    exit(1);
}

$fault = $clReq2->getResponseData($response);
if ($fault != 0) {
    print_r("fault: " . $clReq2->getFaultText() . "\n");
}
if ($response != $req2) {
    print_r("Req2 output is not same as input! [$response] vs [$req2]\n");
    exit(1);
}


// ExecuteAndWaitAny
$r = $pc->executeAndWaitAny();
if ($pc->hasFault()) {
     print_r("[Fault] " . $pc->getFault() . "\n");
     exit(-1);
}

if($r->hasFault()) {
    print_r("[fault] " . $r->getFaultText() . "\n");
    exit(-1);
}

$response = "";

/* Extract the response data */

$fault = $r->getResponseData($response);
if ($fault != 0) {
    print_r("fault: " . $r->getFaultText() . "\n");
}
if ($response != $req2 && $response != $req) {
    print_r("Req2 output is not same as input! [$response] vs [$req2] or [$response] vs [$req2]\n");
    exit(1);
}

?>
