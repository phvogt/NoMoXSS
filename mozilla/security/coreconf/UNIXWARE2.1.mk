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

#
# Config stuff for SCO Unixware 2.1
#

include $(CORE_DEPTH)/coreconf/UNIX.mk

DEFAULT_COMPILER = $(CORE_DEPTH)/build/hcc

CC         = $(CORE_DEPTH)/build/hcc
CCC         = $(CORE_DEPTH)/build/hcpp
RANLIB      = true
OS_CFLAGS   = -KPIC -DSVR4 -DSYSV -DUNIXWARE
MKSHLIB     = $(LD)
MKSHLIB    += $(DSO_LDOPTS)
DSO_LDOPTS += -G
CPU_ARCH    = x86
ARCH        = sco
NOSUCHFILE  = /solaris-rm-f-sucks
ifdef MAPFILE
# Add LD options to restrict exported symbols to those in the map file
endif
# Change PROCESS to put the mapfile in the correct format for this platform
PROCESS_MAP_FILE = cp $(LIBRARY_NAME).def $@

