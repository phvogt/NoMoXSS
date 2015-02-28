#!/usr/bin/perl

# check if two arguments were supplied
if ($#ARGV != 1) { &usage(); }

# get the commandlineparameters
my $goalfile = $ARGV[0];
my $connectionlog = $ARGV[1];

my %goals = &readGoals();

#foreach my $key (keys %goals) {
#	print "goal: " . $key . "\n";
#}

my %results = &readResults();

#foreach my $key (keys %results) {
#	print "result: " . $key . "\n";
#}

# now check the results !

foreach my $key (sort keys %goals) {
	if ($results{$key} == 1) {
		print $key . " ok\n";
	} else {
		print $key . " FAILED!\n";
	}
}


# reads the goal from the $goalfile and returns it in a hash
# parameter: none
# returns a hash with the goals
sub readGoals() {

	my %goals;
	open(IN, "< $goalfile") || die $!;
	while (<IN>) {
		my $line = $_;
		chomp($line);
		$goals{$line} = 1;
	}
	close(IN) || die $!;
  
	return %goals;
}

# reads the results from $connectionlog and returns a hash
# parameter: none
# returns a hash
sub readResults() {

	my %results;
	open(IN, "< $connectionlog") || die $!;
	while (<IN>) {
		my $line = $_;
		chomp($line);
		if ($line =~ /deny/) {
			my ($script) = /\d+\s+\d+\s+\".*\/(.*?)\"/;
			$results{$script} = 1;
#			print "found: " . $script . "\n";
		}
	}
	close(IN) || die $!;
	
	return %results;

}

sub usage() {

print $0. " goalfile connectionlog\n";
exit(0);

}
