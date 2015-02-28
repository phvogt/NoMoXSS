#!/usr/bin/perl


# check if two arguments were supplied
if ($#ARGV != 1) { &usage(); }

# get the commandlineparameters
my $goalfile = $ARGV[0];
my $connectionlog = $ARGV[1];

# some constants
my $PART_NORMAL = 0;
my $PART_IF = 1;
my $PART_EVIL = 2;
my $PART_COMMENT = 3;

my $CHECK_EMPTY = "0";
my $CHECK_DENY = "X";
my $CHECK_IGNORE = "-";

# printf "argv0 = " . $ARGV[0] ." argv1 = " . $ARGV[1] . "\n";
# printf "goalfile = " . $goalfile . " connectionlog = " . $connectionlog . "\n";

my %goals = &readGoals();

# &printGoals(\%goals);

my %results = &readConnectionlog();

#&printGoals(\%results);

&compareGoals(\%goals, \%results);

# reads the goalfile and returns an array with the goals
# parameter: none
# returns: a hash with the jsops as key and the goals as an array
sub readGoals() {
	
	my %result;
	open(GOAL,"< $goalfile") || die $!;
	while(<GOAL>) {
		my $line = $_;
		# print "reading line: " . $line;
		
		
		# split the line at tabs
		my @parts = split(/\t/, $line);
		
#		foreach my $part (@parts) {
#			print "part = " . $part . " ";
#		}
#		print "\n";
		
		# check if we have a valid line
		if ($parts[0] =~ /^JSOP_/) {
#			print "found line: ". $parts[0] . "\n";
			$result{$parts[0]} = [$parts[1], $parts[2], $parts[3], $parts[4]];
		}
	}
	close(GOAL) || die $!;
	
	return %result;
}

# prints the goals
#
# parameter:
#   %goals a hash with the jsops as key and the goals as an array
# returns: nothing
sub printGoals() {

	my %goals = %{$_[0]};
	foreach my $key (sort keys(%goals)) {
		print $key . ": ";
		my @parts = @{$goals{$key}};
# 		print "parts = " . @parts;
		&printGoal(\@parts);
		
		print "\n";
	}

}

# prints a goal
# parameter:
#   @goal the list with a goal
# returns: nothing
sub printGoal() {

	my @goal = @{$_[0]};
	print "normal = " . $goal[$PART_NORMAL] . " if = " . $goal[$PART_IF] . " evil = " . $goal[$PART_EVIL] . " comment = " . $goal[$PART_COMMENT];	
	
	#	foreach my $goal (@goals) {
	#			print "goal = " . $goal . " ";
	#	}

}

# reads connectionlog
#
# parameter:
# returns: # returns: a hash with the jsops as key and the goals as an array from the connectionlog
sub readConnectionlog() {

	my %result;

	open(CONNECTIONLOG, "< $connectionlog") || die $!;
	while(<CONNECTIONLOG>) {
		
		my $line = $_;
		chomp($line);
		
		# get the right line
		if ($line =~ /deny!/) {
#			print "found line: " . $line . "\n";

			# get the parts of the line and store them in variables
			my @parts = $line =~ /^(.*?)\s+(.*?)\s+\"(.*?)\"\:\s+domaincheck: stored always deny!\s+from\s+(.*?)\s+to\s+(.*?)$/;

			my $date = $parts[0];
			my $time = $parts[1];
			my $url = $parts[2];
			my $from = $parts[3];
			my $to = $parts[4];			
#			print "date = " . $date . " time = " . $time . " url = " . $url . " from = " . $from . " to = " . $to . "\n";

			# extract the jsopcode and get the goals
			my ($jsop) = $url =~ /xss_(.*?).pl/;
			$jsop = uc($jsop);
			if ($jsop eq "JSOP_GOSUBXA") { $jsop = "JSOP_GOSUBX"; }
			if ($jsop eq "JSOP_GOTOXA") { $jsop = "JSOP_GOTOX"; }
			if ($jsop eq "JSOP_IFNEXA") { $jsop = "JSOP_IFNEX"; }
# 			print $jsop . " \n";

 			my @resultgoal = @{$result{$jsop}};
			
#			print "resultgoal = " . $#resultgoal;			
			# not yet set: initialize it
			if ($#resultgoal == -1) {
				$resultgoal[$PART_NORMAL] = "0";
				$resultgoal[$PART_IF] = "0";
				$resultgoal[$PART_EVIL] = "0";
			}

			# normal testcase
			if ($to =~ /\?normal=/) {
#				print "found normal: " . $to . "\n";
				$resultgoal[$PART_NORMAL] = "X";
				
				$result{$jsop} = \@resultgoal;
			}
			
			# if testcase
			if ($to =~ /\?if=/) {
#				print "found if: " . $to . "\n";
				$resultgoal[$PART_IF] = "X";
				$result{$jsop} = \@resultgoal;
			}
			
			# evil testcase
			if ($to =~ /\?isevil=/) {
# 				print "found isevil: " . $to . " \n";
				$resultgoal[$PART_EVIL] = "X";
				$result{$jsop} = \@resultgoal;
			}
#			&printGoal(\@resultgoal); print "\n";
			
		}
		
	}

	close(CONNECTIONLOG) || die $!;
	
	return %result;
}

sub compareGoals() {

	my %goals = %{$_[0]};
	my %results = %{$_[1]};

#	&printGoals(\%goals);
#	&printGoals(\%results);
	foreach my $jsop (sort keys(%goals)) {
		my @goal = @{$goals{$jsop}};
#		&printGoal(\@goal);
		my $goalnormal = $goal[$PART_NORMAL];
		my $goalif = $goal[$PART_IF];
		my $goalevil = $goal[$PART_EVIL];
		
		my @resultgoals = @{$results{$jsop}};
		my $resultnormal = $resultgoals[$PART_NORMAL] || "0";
		my $resultif = $resultgoals[$PART_IF] || "0";
		my $resultevil = $resultgoals[$PART_EVIL] || "0";
		
		my $testresult = 1;
		if (($goalnormal ne $CHECK_IGNORE) && ($goalnormal ne $resultnormal)) {
			$testresult = 0;
		}
		if (($goalif ne $CHECK_IGNORE) && ($goalif ne $resultif)) {
			$testresult = 0;
		}
		if (($goalevil ne $CHECK_IGNORE) && ($goalevil ne $resultevil)) {
			$testresult = 0;
		}
		
		print sprintf("%-20s", $jsop) . ": |" . $goalnormal . " " . $goalif . " " . $goalevil . "|  |". $resultnormal . " " . $resultif . " " . $resultevil . "|";
		if ($testresult == 1) {
			print " TEST OK ";
		} else {
			print " TEST ERROR ";
		}
		print "\n";
	}
}

sub usage() {

print $0. " goalfile connectionlog\n";
exit(0);

}
