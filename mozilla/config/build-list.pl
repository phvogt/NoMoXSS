#!env perl

#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 2001 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 
#	Christopher Seawood <cls@seawood.org>

#
# A generic script to add entries to a file 
# if the entry does not already exist
# 
# Usage: $0 [-l] <filename> <entry> [<entry> <entry>]
#
#   -l do not attempt flock the file.

use Fcntl qw(:DEFAULT :flock);
use Getopt::Std;
use mozLock;

sub usage() {
    print "$0 [-l] <filename> <entry>\n";
    exit(1);
}

$nofilelocks = 0;

getopts("l");

$nofilelocks = 1 if defined($::opt_l);

$file = shift;

undef @entrylist;
while ($entry = shift) {
    push @entrylist, $entry;
}

$lockfile = $file . ".lck";

# touch the file if it doesn't exist
if ( ! -e "$file") {
    $now = time;
    utime $now, $now, $file;
}

# This needs to be atomic
mozLock($lockfile) unless $nofilelocks;

# Read entire file into mem
undef @inbuf;
if ( -e "$file" ) {
    open(IN, "$file") || die ("$file: $!\n");
    binmode(IN);
    while ($tmp = <IN>) {
	chomp($tmp);
	push @inbuf, $tmp;
    }
    close(IN);
}

undef @outbuf;
# Add each entry to file if it's not already there
foreach $entry (@entrylist) {
    push @outbuf, $entry if (!grep(/^$entry$/, @inbuf));
}

$count = $#outbuf + 1;

# Append new entry to file
if ($count) {
    open(OUT, ">>$file") || die ("$file: $!\n");
    binmode(OUT);
    foreach $entry (@outbuf) {
	print OUT "$entry\n";
    }
    close(OUT);
}

mozUnlock($lockfile) unless $nofilelocks;

exit(0);
