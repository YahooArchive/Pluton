#! /usr/local/bin/perl -w

use Yahoo::pluton;

print "Client API: ", Yahoo::pluton::client::getAPIVersion(), "\n";
print "Service APU: ", Yahoo::pluton::service::getAPIVersion(), "\n";

my $s = Yahoo::pluton::service->new("perlbaby");
$s->initialize();

while ($s->getRequest()) {
    my $str = $s->getRequestData();
#    print STDERR "Have req of $str\n";
    if ($s->hasFault()) {
	print STDERR "getRequestData Fault: ", $s->getFault()->getMessage("getRequest"), "\n";
	exit(0);
    }

    if (length($str) == 0) {
	$s->sendFault(1, "Empty request is invalid");
    }
    else {
	my $response = "Your request is " . length($str) . " bytes long";
	$s->sendResponse($response);
    }
}

if ($s->hasFault()) {
    print STDERR "Service Fault: ", $s->getFault()->getMessage("getRequest"), "\n";
    exit(0);
}

$s->terminate();
exit(0);

