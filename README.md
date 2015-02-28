# NoMoXSS
No More XSS attacks - master thesis example of tracking sensitive data by using JavaScript data tainting for Firefox 0.10

Cross site scripting (XSS) is a common security problem of web applications where an attacker can inject scripting code into the output of the application that is then sent to a user's web browser. In the browser, this scripting code is executed and used to transfer sensitive data to a third party. Todays solutions attempt to prevent XSS on the server side, for example, by inspecting and modifying the data sent to and from the web application. The presented solution, on the other hand, stops XSS attacks on the client side by tracking the use of sensitive information in the JavaScript engine of the web browser. If sensitive information is about to be transferred to a third party, the user can decide if this should be allowed or not. As a result, the user has an additional protection layer when surfing websites without solely depending on the security of the web application.

## Documentation / Publications

"NoMoXSS" is the software prototype of the master thesis (~1Mb) "Cross Site Scripting (XSS) Attack Prevention with Dynamic Data Tainting on the Client Side"

This paper (~800kb) gives an overview of the approach.
A project website is available at the Secure Systems Lab.

Prerequisites

* [The Mozilla Firefox source](http://ftp.mozilla.org/pub/mozilla.org/firefox/releases/0.10.1/firefox-1.0PR-source.tar.bz2)
* The necessary patch file
* A build system (e.g., Windows/Linux)
  * Windows XP Professional Service Pack 2, Visual Studio 2003 (later Visual Studio 2005 Express), and Cygwin
  * Debian Sarge
  * Mandriva Linux 10.2 (Limited Edition 2005)

## Install

Read the build documentation. 
