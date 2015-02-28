#!/usr/bin/perl
while(<>) {
  my $line = $_;
  chomp($line);
  $execstr = "firefox -remote \"openURL(http://wintermute.wg.vlbg/cgi-bin/$line)\"";
  print $execstr."\n";
  system($execstr);
  sleep(7);
}
