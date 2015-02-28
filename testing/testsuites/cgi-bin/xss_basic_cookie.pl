$query = new CGI;
$cookie = $query->cookie(-name=>$cookiename,
   -value=>$cookievalue,
   -expires=>'+1h',
);

print $query->header(
  -cookie=>$cookie
);

1;