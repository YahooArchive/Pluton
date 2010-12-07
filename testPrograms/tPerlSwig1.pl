#! /usr/bin/perl -w

use Yahoo::pluton;

print Yahoo::pluton::client::getAPIVersion(), "\n";

my $c = Yahoo::pluton::client->new("tPerlSwig1");
$c->initialize("");
# $c->setDebug(1);

my $r1 = Yahoo::pluton::clientRequest->new();
my $r2 = Yahoo::pluton::clientRequest->new();

my $rd1 = "123456789A123456789A12345X";
$r1->setRequestData("123456789A123456789A12345X");

my $rd2 = "123456789A123456789A12345X" x 4;
$r2->setRequestData($rd2);
$r2->setContext("echo.sleepMS", "500");
print "Requests set\n";

if (! $c->addRequest("system.echo.0.raw", $r1)) {
    print $c->getFault()->getMessage("add Request"), "\n";
    exit(1);
}

if (! $c->addRequest("system.echo.0.raw", $r2)) {
    print $c->getFault()->getMessage("add Request2"), "\n";
    exit(1);
}

$c->executeAndWaitAll();
if ($c->hasFault()) {
    print $c->getFault()->getMessage("E&WAll"), "\n";
    exit(1);
}

if ($r1->hasFault()) {
    print "r1 faultCode: " . $r1->getCode() . " tx: " . $r1->getFaultText();
    print " Service: " . $r1->getServiceName() . "\n";
    exit(1);
}
my $s = $r1->getResponseData();
if ($s ne $rd1) {
    print "r1 mis-matches $s vs $rd1\n";
    exit(1);
}
print "r ok from " . $r1->getServiceName() . "\n";

if ($r2->hasFault()) {
    print "r2 faultCode: " . $r->getCode() . " tx: " . $r->getFaultText();
    print " Service: " . $r->getServiceName() . "\n";
    exit(1);
}

$s = $r2->getResponseData();
if ($s ne $rd2) {
    print "Response mis-matches 2 $s vs $rd2\n";
    exit(1);
}
print "r2 ok from ", $r2->getServiceName(), "\n";

# Check wany

$r2->setRequestData($rd2);
$c->addRequest("system.echo.0.raw", $r2);
$c->executeAndWaitAny();
if ($c->hasFault()) {
    print $c->getFault()->getMessage("E&WAny"), "\n";
    exit(1);
}

if ($r2->hasFault()) {
    print "r2.a faultCode: " . $r->getCode() . " tx: " . $r->getFaultText();
    print " Service: " . $r->getServiceName() . "\n";
    exit(1);
}
$s = $r2->getResponseData();
if ($s ne $rd2) {
    print "Response mis-matches 2.a $s vs $rd2\n";
    exit(1);
}
print "r2.a ok from ", $r2->getServiceName(), "\n";

# Check timeout

$c->setTimeoutMilliSeconds(1000);
$r2->setRequestData($rd2);
$r2->setContext("echo.sleepMS", "2000");
if (! $c->addRequest("system.echo.0.raw", $r2)) {
    print $c->getFault()->getMessage("Add r2.b"), "\n";
    exit(1);
}

$c->executeAndWaitOne($r2);
if ($c->hasFault()) {
    print $c->getFault()->getMessage("E&WOne"), "\n";
    exit(1);
}
if ($r2->hasFault()) {
    print "r2.b faultCode: " . $r2->getFaultCode() . " tx: " . $r2->getFaultText();
    print " Service: " . $r2->getServiceName() . "\n";
    exit(0) if $r2->getFaultCode() == -19;
}

print "Error: r2.b did not time out\n";
exit(1);

