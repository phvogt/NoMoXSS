#!/usr/bin/perl

require "xss_basic_vars.pl";

$title = "Test window.defaultStatus";
$dut = "window.defaultStatus";

require "xss_basic_uses.pl";

require "xss_basic_cookie.pl";

require "xss_basic_header.pl";

require "xss_basic_bodystart.pl";

require "xss_basic_form.pl";

require "xss_basic_normaltest.pl";

require "xss_basic_footer.pl";
