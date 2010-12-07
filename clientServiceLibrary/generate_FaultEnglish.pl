#! /usr/bin/perl -w

# Feed fault.h via stdin and generate a C array on stdout

print <<EOD;

////////////////////////////////////////
// DO NOT EDIT
// This code is auto-generated
////////////////////////////////////////

static const char*
faultCodeToEnglish(pluton::faultCode fc)
{
    switch(fc) {
EOD

while (<>) {
    next unless /\s+(\S+) \=\s+\S+\s+\/\/ E: (.*)/;
    printf("\tcase pluton::%-30s: return \"%s:%s\";\n", $1, $1, $2);
}

print "\tdefault: break;\n";

print <<EOD;
    }
    return "Unknown faultCode?";
}
EOD

