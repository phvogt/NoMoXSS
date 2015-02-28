# 
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
# 
# The Original Code is the Netscape security libraries.
# 
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are 
# Copyright (C) 1994-2000 Netscape Communications Corporation.  All
# Rights Reserved.
# 
# Contributor(s):
# 
# Alternatively, the contents of this file may be used under the
# terms of the GNU General Public License Version 2 or later (the
# "GPL"), in which case the provisions of the GPL are applicable 
# instead of those above.  If you wish to allow use of your 
# version of this file only under the terms of the GPL and not to
# allow others to use your version of this file under the MPL,
# indicate your decision by deleting the provisions above and
# replace them with the notice and other provisions required by
# the GPL.  If you do not delete the provisions above, a recipient
# may use your version of this file under either the MPL or the
# GPL.
#

#######################################################################
# Set the LDFLAGS value to encompass all normal link options, all     #
# library names, and all special system linking options               #
#######################################################################

LDFLAGS = \
	$(DYNAMIC_LIB_PATH) \
	$(LDOPTS) \
	$(LIBSECTOOLS) \
	$(LIBSECMOD) \
	$(LIBHASH) \
	$(LIBCERT) \
	$(LIBKEY) \
	$(LIBCRYPTO) \
	$(LIBSECUTIL) \
	$(LIBDBM) \
	$(LIBPLC3) \
	$(LIBPLDS3) \
	$(LIBPR3) \
	$(DLLSYSTEM) \
	$(LIBJAR) \
	$(LIBZLIB) \
	$(LIBPKCS7) \
	$(LIBPLC3)

# Strip out the symbols
ifdef BUILD_OPT
    ifneq (,$(filter-out WIN%,$(OS_TARGET)))
	LDFLAGS += -s
    endif
endif

#######################################################################
# Adjust specific variables for all platforms                         #
#######################################################################
 

ifeq (,$(filter-out WIN%,$(OS_TARGET)))
    PACKAGE_FILES = license.txt README.TXT specification.html pk11jar.html modutil.exe
else
    PACKAGE_FILES = license.doc README specification.html pk11jar.html modutil
endif

ARCHIVE_NAME = modutil_$(OS_TARGET)$(OS_RELEASE)
