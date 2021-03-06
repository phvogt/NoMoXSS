#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_EQ";
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
			evil = null;
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = evil;
		var dut = { };
		var dut2 = { };
		var dut3 = { };

		// shouldn't be evil 

		notevil = dut == null;
		
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			notevil2 = dut2 == null;
		}
		
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;
		
		// normal usage 	
		
		notevil3 = dut3 == evil;
		var url3 = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
