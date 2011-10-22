#! /usr/local/bin/perl -w

use Yahoo::pluton;

print Yahoo::pluton::client::getAPIVersion(), "\n";

my $c = Yahoo::pluton::client->new("perlbaby");
$c->initialize("");
# $c->setDebug(1);

my $r = Yahoo::pluton::clientRequest->new();

my $rd = "123456789A123456789A12345X";
$r->setRequestData($rd);

print "Request set\n";

if (! $c->addRequest("system.echo.0.raw", $r)) {
    print $c->getFault()->getMessage("add Request"), "\n";
    exit(1);
}

my $newR = $r;
my $newC = $c;

$c->executeAndWaitAll();
if ($c->hasFault()) {
    print $c->getFault()->getMessage("E&W"), "\n";
    exit(1);
}

my $s = $r->getResponseData();

print "Response Data: ", $s, "\n";
