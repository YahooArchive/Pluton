#! /usr/bin/perl -w

# Feed fault.h via stdin and generate an exception switch list for JNI

print <<EOD;

/**************************************************
 * DO NOT EDIT
 * This code is auto-generated
 **************************************************/

static const char*
faultCodeToException(int fc)
{
    switch(fc) {
EOD

#     deserializeFailed = 1,              // E: De-serialization of netString packet failed

while (<>) {

    printf("\tcase %d: return \"com/yahoo/pluton/%s\";\n", $2, $1)
	if (/\s+(\S+)\s+\=\s+(\d+),\s+\/\/ E:/);

    printf("\tcase -%d: return \"com/yahoo/pluton/%s\";\n", $2, $1)
	if (/\s+(\S+)\s+\=\s+\-(\d+),\s+\/\/ E:/);
}

print <<EOD;

	default: break;
    }
    return "com/yahoo/pluton/unKnownFault";
}
EOD
