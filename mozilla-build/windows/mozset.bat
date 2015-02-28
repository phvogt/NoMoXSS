call "D:\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat"

set MOZ_TOOLS=d:\mozilla-src\moztools
set PATH=d:\mozilla-src\vc71\bin;d:\mozilla-src\vc71\lib;%MOZ_TOOLS%\bin;d:\cygwin\bin;%PATH%;
set MOZCONFIG=d:\mozilla-src\.mozconfig
set MOZ_DEBUG=1
set DISABLE_TESTS=
set LIB=D:\mozilla-src\vc71\lib;D:\mozilla-src\vc71\bin;%LIB%
set INCLUDE=D:\mozilla-src\vc71\include;D:\mozilla-src\vc71\include\libIDL;%INCLUDE%
set VERBOSE=1
set CVSROOT=:pserver:anonymous@cvs-mirror.mozilla.org:/cvsroot

rem make -f client.mk build >>..\build.log 2>&1