#!/usr/bin/perl

my @files = `ls cgi-bin`;

foreach my $file (@files) {

	chomp($file);
	if ($file =~ /xss_jsop.*\.pl/) {

		print "found file: ". $file . "\n";
		
		&replaceInFile("cgi-bin/". $file, "cgi-binnew/". $file);
  	}

} 

# replaces the document.images[0].src in file
# parameter:
#   file the name of the file to read
#   outfile the name of the file to write to
sub replaceInFile() {

	my $file = $_[0];
	my $outfile = $_[1];
	
	open(IN, "< $file") || die $!;
	open(OUT, "> $outfile") || die $!;
	my $count = 0;
	while (<IN>) {
		my $line = $_;
		if ($line =~ /document\.images\[0\]\.src/) {
			
			if ($count > 0) {
				$line =~ s/document\.images\[0\]\.src/document\.images\[$count\].src/i;
			}
			print OUT $line;
			++$count;
		} else {
		  print OUT $line;
		}
	
	}
	
	close(OUT) || die $!;
	close(IN) || die $!;

}
