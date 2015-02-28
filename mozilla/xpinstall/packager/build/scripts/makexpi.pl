#!c:\perl\bin\perl
# 
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#  
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#  
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
# 
# The Initial Developer of the Original Code is Netscape
# Communications Corporation. Portions created by Netscape are
# Copyright (C) 1998-1999 Netscape Communications Corporation. All
# Rights Reserved.
# 
# Contributor(s): 
# Sean Su <ssu@netscape.com>
# 

#
# This perl script creates .xpi files given component input name
#
# Input: component name
#             - name of the component directory located in the staging path
#        staging path
#             - path to where the built files are staged at
#        dest path
#             - path to where the .xpi files are are to be created at.
#               ** MUST BE AN ABSOLUTE PATH, NOT A RELATIVE PATH **
#
#   ie: perl makexpi.pl xpcom z:\exposed\windows\32bit\en\5.0 d:\build\mozilla\dist\win32_o.obj\install\working
#

use File::Copy;
use Cwd;

# Make sure there are at least three arguments
if($#ARGV < 3)
{
  die "usage: $0 <component name> <staging path> <dest path>

       component name : name of component directory within staging path
       js dir         : path the directory where the .js file lives
       staging path   : path to where the components are staged at
       dest path      : path to where the .xpi files are to be created at
       \n";
}

$inComponentName  = $ARGV[0];
$inJsDir          = $ARGV[1];
$inStagePath      = $ARGV[2];
$inDestPath       = $ARGV[3];

$inStagePath      =~ s/\//\\/g;
$inDestPath       =~ s/\//\\/g;

print "\n Making $inComponentName.xpi...\n";
$saveCwdir = cwd();

# check for existance of staging component path
if(!(-e "$inStagePath\\$inComponentName"))
{
  die "invalid path: $inStagePath\\$inComponentName\n";
}

if($inComponentName =~ /xpcom/i)
{
  # copy msvcrt.dll to xpcom dir
  if(-e "$ENV{MOZ_SRC}\\redist\\microsoft\\system\\msvcrt.dll")
  {
    system("copy $ENV{MOZ_SRC}\\redist\\microsoft\\system\\msvcrt.dll  $inStagePath\\$inComponentName");
  }

  # copy msvcirt.dll to xpcom dir
  if(-e "$ENV{MOZ_SRC}\\redist\\microsoft\\system\\msvcirt.dll")
  {
    system("copy $ENV{MOZ_SRC}\\redist\\microsoft\\system\\msvcirt.dll $inStagePath\\$inComponentName");
  }
}

# check for existance of .js script
if(!(-e "$inJsDir\\$inComponentName.js"))
{
  die "missing .js script: $inJsDir\\$inComponentName.js\n";
}

# delete component .xpi file
if(-e "$inDestPath\\$inComponentName.xpi")
{
  unlink("$inDestPath\\$inComponentName.xpi");
}
if(-e "$inStagePath\\$inComponentName\\$inComponentName.xpi")
{
  unlink("$inDestPath\\$inComponentName.xpi");
}

# make sure inDestPath exists
if(!(-d "$inDestPath"))
{
  system("mkdir $inDestPath");
}

# delete install.js
if(-e "$inStagePath\\$inComponentName\\install.js")
{
  unlink("$inStagePath\\$inComponentName\\install.js");
}
copy("$inJsDir\\$inComponentName.js", "$inStagePath\\$inComponentName\\install.js");

# change directory to where the files are, else zip will store
# unwanted path information.
chdir("$inStagePath\\$inComponentName");
if(system("zip -r $inDestPath\\$inComponentName.xpi *"))
{
  chdir("$saveCwdir");
  die "\n Error: zip -r $inDestPath\\$inComponentName.xpi *\n";
}
chdir("$saveCwdir");

if($inComponentName =~ /^jrereg$/i)
{
  # Remove the README file that is not part of the build.
  system("$ENV{MOZ_TOOLS}\\bin\\zip -d $inDestPath\\$inComponentName.xpi README");
}

# delete install.js
if(-e "$inStagePath\\$inComponentName\\install.js")
{
  unlink("$inStagePath\\$inComponentName\\install.js");
}

print "\n $inComponentName.xpi done!\n";

# end of script
exit(0);

