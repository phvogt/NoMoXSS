#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test select.selectedIndex";
$dut = "document.".$testform.".testselect.selectedIndex";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

print qq|
    <script type="text/javascript">
		document.| . $testform . qq|.testselect.selectedIndex = 1;
    </script>
|;

require "xss_basic_normaltest.pl";

require "xss_basic_footer.pl";
