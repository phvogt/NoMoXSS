#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_DUP2";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		// shouldn't be evil 
		var notevil = [2];
		notevil[0] += 1;
		var url = "$imgevilurl?normal=" + notevil[0];
		document.images[0].src = url;
		
		// if with evil val 
		var notevil2 = [2];
		if ($evil) {
			notevil2[0] += 1;
		}
		var url2 = "$imgevilurl?if=" + notevil2[0];
		document.images[1].src = url2;
		
		// normal usage 
		var notevil3 = [2];
		var evil = 2;
		if ($evil) {
			evil = 0;
		}
		notevil3[evil] += 1;
		var url3 = "$imgevilurl?isevil=" + notevil3[0];
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
