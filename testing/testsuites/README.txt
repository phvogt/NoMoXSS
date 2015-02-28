- cgi-bin

contains all scripts that have to be deployed on the testserver (e.g., accessable at http://wintermute.wg.vlbg/cgi-bin
  xss_jsop*.pl are the opcode tests
  xss_basic*.pl are the scripts for the testframework
  xss_other*.pl are scripts to test complex behaviour

- jsops

contains the tests for the opcodes

- sources

contains the tests for the initial tainted sources

- fix_pics.pl 

is a script to change the image-sources in the scripts (no longer needed)