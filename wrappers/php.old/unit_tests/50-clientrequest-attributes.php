#!/usr/local/bin/php
<?php

function abortme($why, $excode)
{
  print "Attributes1 failed: $why\n";
  exit($excode);

}

$client = new PlutonClient('attributes');

$api = $client->getAPIVersion();

print "API=$api\n\n";

print "Setting attributes\n";
$request = new PlutonClientRequest();
$request->setAttribute(PLUTON_NOWAIT_ATTR);
$request->setAttribute(PLUTON_NOREMOTE_ATTR);
$request->setAttribute(PLUTON_NORETRY_ATTR);
$request->setAttribute(PLUTON_KEEPAFFINITY_ATTR);
$request->setAttribute(PLUTON_NEEDAFFINITY_ATTR);
$request->setAttribute(PLUTON_ALL_ATTR);

print "Fetching attributes\n";

function checkAttribute($req, $name, $att, $val)
{
  $b = $req->getAttribute($att);
  if ($b != $val) {
    abortme("Attribute not as expected for $name as $b != $val", 1);
  }
  print "$name=$b ok\n";
}

checkAttribute($request, "PLUTON_NOWAIT_ATTR", PLUTON_NOWAIT_ATTR, 1);
checkAttribute($request, "PLUTON_NOREMOTE_ATTR", PLUTON_NOREMOTE_ATTR, 1);
checkAttribute($request, "PLUTON_NORETRY_ATTR", PLUTON_NORETRY_ATTR, 1);
checkAttribute($request, "PLUTON_KEEPAFFINITY_ATTR", PLUTON_KEEPAFFINITY_ATTR, 1);
checkAttribute($request, "PLUTON_NEEDAFFINITY_ATTR", PLUTON_NEEDAFFINITY_ATTR, 1);
checkAttribute($request, "PLUTON_ALL_ATTR", PLUTON_ALL_ATTR, 1);

print "Checking one off\n";
$request->clearAttribute(PLUTON_NOWAIT_ATTR);
checkAttribute($request, "PLUTON_NOWAIT_ATTR", PLUTON_NOWAIT_ATTR, 0);

print "Checking all off\n";
$request->clearAttribute(PLUTON_ALL_ATTR);
checkAttribute($request, "PLUTON_NOWAIT_ATTR", PLUTON_NOWAIT_ATTR, 0);
checkAttribute($request, "PLUTON_NOREMOTE_ATTR", PLUTON_NOREMOTE_ATTR, 0);
checkAttribute($request, "PLUTON_NORETRY_ATTR", PLUTON_NORETRY_ATTR, 0);
checkAttribute($request, "PLUTON_KEEPAFFINITY_ATTR", PLUTON_KEEPAFFINITY_ATTR, 0);
checkAttribute($request, "PLUTON_NEEDAFFINITY_ATTR", PLUTON_NEEDAFFINITY_ATTR, 0);
checkAttribute($request, "PLUTON_ALL_ATTR", PLUTON_ALL_ATTR, 0);

?>

