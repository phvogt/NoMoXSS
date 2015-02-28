#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_DEFAULT";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		// shouldn't be evil 
		var notevil = 1;
		switch(notevil) {
			case (notevil == 3):
				notevil = 2;
				break;
			default:
				notevil = 3;
		}		
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		var evil = 2;

		// if with evil val 
		var notevil2 = 2;
		var url2 = "$imgevilurl?if=";
		if ($evil) {
			switch(notevil2) {
				case (evil == 3):
					notevil3 = 2;
					break;
				default:
					notevil2 = 3;
					url2 = url2 + notevil2;
			}
		}
		document.images[1].src = url2;
		
		// normal usage 
		if ($evil) {
			evil = 4;
		}
		var notevil3 = 1;
		switch(notevil3) {
			case (evil == 3):
				notevil3 = 2;
				break;
			default:
				notevil3 = 3;
		}
		document.images[2].src = "$imgevilurl?isevil=" + notevil3;
		
    </script>
|;

require "xss_basic_footer.pl";


