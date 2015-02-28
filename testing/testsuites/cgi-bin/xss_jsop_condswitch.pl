#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_CONDSWITCH";
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
		switch(1) {
		  case (notevil == 1) :
			  notevil = 4;
			  break;
		}
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val
		var notevil2 = 1;
		if ($evil) {
			switch(1) {
			  case (notevil2 == 1) :
				notevil2 = 4;
				break;
			}
		}
		var url = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url;
	
		// normal usage
		var notevil3 = 1;
		var evil;
		if ($evil) {
		  evil = true;
		}
		switch(evil) {
		  case (notevil3 == 1) :
			notevil3 = 3;
			break;
		}
		document.images[2].src = "$imgevilurl?isevil=" + notevil3;
    </script>
|;

require "xss_basic_footer.pl";
