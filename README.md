# NoMoXSS
No More XSS attacks - master thesis example of tracking sensitive data by using JavaScript data tainting for Firefox 0.10

## Disclaimer

This project is based on Firefox 0.10! It was my master thesis and is only a proof of concept. Using this browser does not provide more security because the used browser Firefox 0.10 is outdated. Please consider using a current version of [Firefox](https://www.mozilla.org/en-US/firefox/new/) for daily usage.

If you are interested about cross site scripting (XSS) and the technique of data tainting for tracking sensitive data while processing JavaScript, read on!

## Overview

Cross site scripting (XSS) is a common security problem of web applications where an attacker can inject scripting code into the output of the application that is then sent to a user's web browser. In the browser, this scripting code is executed and used to transfer sensitive data to a third party. Todays solutions attempt to prevent XSS on the server side, for example, by inspecting and modifying the data sent to and from the web application. The presented solution, on the other hand, stops XSS attacks on the client side by tracking the use of sensitive information in the JavaScript engine of the web browser. If sensitive information is about to be transferred to a third party, the user can decide if this should be allowed or not. As a result, the user has an additional protection layer when surfing websites without solely depending on the security of the web application.

## Documentation / Publications

"NoMoXSS" is the software prototype of the master thesis (~1Mb) "Cross Site Scripting (XSS) Attack Prevention with Dynamic Data Tainting on the Client Side"

This paper (~800kb) gives an overview of the approach.
A project website is available at the Secure Systems Lab.

###Prerequisites

* From scratch:
  * [The Mozilla Firefox source](http://ftp.mozilla.org/pub/mozilla.org/firefox/releases/0.10.1/firefox-1.0PR-source.tar.bz2)
  * The necessary [patch file](mozilla-patch/xss.patch)
* or you use:
  * the [already patched mozilla](mozilla/)
* A build system (e.g., Windows/Linux). Tested with:
  * Windows XP Professional Service Pack 2, Visual Studio 2003 (later Visual Studio 2005 Express), and Cygwin
  * Debian Sarge
  * Mandriva Linux 10.2 (Limited Edition 2005)

## Build

###Windows Build

Read the mozilla.org instructions first: [Windows Build Prerequisites on the 1.7 and 1.8 Branches - MDC](http://developer.mozilla.org/en/docs/Windows_Build_Prerequisites_on_the_1.7_and_1.8_Branches)

I used the following directory structure:

* `d:\mozilla-src` is the main directory
* `d:\mozilla-src\mozilla` contains the sources
* `d:\mozilla-src\moztools` contains the moztools package
* `d:\mozilla-src\vc71` contains glib/libIDL for MSVC7/7.1
* `D:\Microsoft Visual Studio .NET 2003` contains the installed version of Visual Studio 2003
* `d:\cygwin contains `the installation of Cygwin

Use the following cygwin packages (I had problems with newer packages):

* gcc: 3.3.3
* make: 3.7

To build the web browser use the following steps:

* Extract the Firefox source to d:\mozilla-src\mozilla
* Copy the patch file to d:\mozilla-src
* Start a cygwin shell, change to d:\mozilla-src\mozilla directory
* Test the patch:

`/usr/bin/patch.exe -p1 -u --dry-run < mozilla-patch/xss.patch`

* If everything looks good, apply the patch.

`/usr/bin/patch.exe -p1 -u < mozilla-patch/xss.patch`

* Create `d:\mozilla-src\.mozconfig` (use cygwin to create a file starting with a dot!)
* Exit the cygwin shell and start a windows command shell (cmd.exe)
* Copy [mozset.bat](mozilla-build/windows/mozset.bat) to `d:\mozilla-src\` (adapt it if necessary) and call it to set the environment variables
* Create `d:\mozilla-src\mozilla\browser\config\mozconfig` file, see [browser_mozconfig](mozilla-build/windows/browser_mozconfig).
* Remove or adapt the line

`MOZ_OBJDIR = d:/mozilla-src/mozilla/firefox_obj_dir`

in `d:\mozilla-src\mozilla\client.mk`
* Start the build process in a Windows command window with:

`make -f client.mk build`

* To clean use:

`make -f client.mk distclean`


If you do not want a debug-build remove the line:

`    set MOZ_DEBUG=1`

 the mozset.bat and remove the line

`    ac_add_options --enable-debug`

from `d:\mozilla-src\.mozconfig`

###Linux Build

Read the mozilla.org instructions first: [Build and Install - MDC](http://developer.mozilla.org/en/docs/Build_and_Install)

####Debian Sarge
I used Debian Sarge.
  * [Debian Sarge i386 netinst ISO](http://cdimage.debian.org/cdimage/archive/3.1_r8/i386/iso-cd/debian-31r8-i386-netinst.iso)
  * The packages are in http://archive.debian.org/debian/dists/sarge/
  * in `/etc/apt/sources.list`:
  
`deb http://archive.debian.org/debian sarge main contrib non-free`

####Other linux

I unsuccessfully tried to build it with Debian Wheezy. There are problems with versions of gcc and libraries.

####Build
For a Linux build use GTK2 and XFT in the `mozconfig`, because GTK was very unstable in my tests.
and do the following steps:

* If you use Debian Sarge
  * install the packages [mozilla-build/debian-sarge/dpkg.list](mozilla-build/debian-sarge/dpkg.list)
  * use the [mozilla-build/debian-sarge/mozconfig](mozilla-build/debian-sarge/mozconfig) 
* Set the environment variable for `mozconfig` (e.g., `MOZCONFIG=~/build/mozilla-src/mozilla/mozconfig`)
* Remove or adapt the line

`MOZ_OBJDIR = d:/mozilla-src/mozilla/firefox_obj_dir`

in `client.mk`
* Start the build with

`make -f client.mk build`



###Compile errors

If the following error occurs, make sure that you have set the `MOZ_OBJDIR` in `client.mk` or remove the line!

`make -f client.mk build client.mk:760: *** multiple target patterns. Stop.`

When building the web browser, the following error may stop the build process if an object directory   with `MOZ_OBJDIR` is set:

`In file included from ../../../../js/src/xsstaint.h:4, from ../../../dist/include/string/nsTAString.h:41, from ../../../dist/include/string/nsAString.h:57, from /home/mozilla-src/mozilla/xpcom/string/src/nsAString.cpp:39: ../../../../js/src/jstypes.h:221:71: jsautocfg.h: No such file or directory`

**Solution**: Copy the `jsautocfg.h` from `MOZ_OBJDIR/js/src/jsautocfg.h` to `$topsrcdir/js/src` directory (`$topsrcdir` refers to the directory where the Mozilla Firefox source can be found).

##Testing

There is a [Testsuite](testing) that contains the basic tests and the exploits with instructions. 
